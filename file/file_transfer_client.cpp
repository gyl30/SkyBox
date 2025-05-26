#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/asio/redirect_error.hpp>
#include "log/log.h"
#include "crypt/random.h"
#include "protocol/message.h"
#include "file/file_transfer_client.h"

#include <utility>

namespace leaf
{

file_transfer_client::file_transfer_client(std::string ip, uint16_t port, std::string username, std::string password, leaf::progress_handler handler)
    : id_(leaf::random_string(8)),
      host_(std::move(ip)),
      port_(std::to_string(port)),
      user_(std::move(username)),
      pass_(std::move(password)),
      handler_(std::move(handler))
{
}

file_transfer_client::~file_transfer_client()
{
    //
    LOG_INFO("{} shutdown", id_);
}

boost::asio::awaitable<void> file_transfer_client::login(boost::system::error_code &ec)
{
    leaf::login_request login_request;
    login_request.username = user_;
    login_request.password = pass_;
    auto data = leaf::serialize_login_request(login_request);
    std::string target = "/leaf/login";
    boost::beast::http::request<boost::beast::http::string_body> req;
    req.version(11);
    req.method(boost::beast::http::verb::post);
    req.target(target);
    req.set(boost::beast::http::field::host, host_);
    req.set(boost::beast::http::field::content_type, "application/json");
    req.set(boost::beast::http::field::user_agent, "leaf/http");
    req.body() = std::string(data.begin(), data.end());
    req.prepare_payload();
    auto resolver = boost::asio::use_awaitable_t<boost::asio::any_io_executor>::as_default_on(
        boost::asio::ip::tcp::resolver(co_await boost::asio::this_coro::executor));

    auto const results = co_await resolver.async_resolve(host_, port_);

    leaf::tcp_stream_limited stream(co_await boost::asio::this_coro::executor);
    stream.expires_after(std::chrono::seconds(30));
    auto ep = co_await stream.async_connect(results, boost::asio::redirect_error(boost::asio::use_awaitable, ec));
    if (ec)
    {
        LOG_ERROR("{} connect {}:{} failed {}", id_, host_, port_, ec.message());
        co_return;
    }
    std::string host = host_ + ':' + std::to_string(ep.port());
    LOG_INFO("{} connect {} success", id_, host);
    stream.expires_after(std::chrono::seconds(30));
    co_await boost::beast::http::async_write(stream, req, boost::asio::redirect_error(boost::asio::use_awaitable, ec));
    if (ec)
    {
        LOG_ERROR("{} write to {} failed {}", id_, host, ec.message());
        co_return;
    }
    boost::beast::flat_buffer buffer;
    boost::beast::http::response<boost::beast::http::vector_body<uint8_t>> res;
    stream.expires_after(std::chrono::seconds(30));
    co_await boost::beast::http::async_read(stream, buffer, res, boost::asio::redirect_error(boost::asio::use_awaitable, ec));
    if (ec)
    {
        LOG_ERROR("{} read from {} failed {}", id_, host, ec.message());
        co_return;
    }
    auto login_response = leaf::deserialize_login_token(res.body());
    if (!login_response)
    {
        LOG_ERROR("{} login deserialize failed", id_);
        co_return;
    }
    token_ = login_response->token;
    ec = stream.socket().close(ec);
    if (ec)
    {
        LOG_ERROR("{} close socket failed {}", id_, ec.message());
        co_return;
    }
}
boost::asio::awaitable<void> file_transfer_client::loop_coro()
{
    timer_ = std::make_shared<boost::asio::steady_timer>(co_await boost::asio::this_coro::executor, std::chrono::seconds(5));
    boost::system::error_code ec;
    while (true)
    {
        co_await timer_->async_wait(boost::asio::redirect_error(boost::asio::use_awaitable, ec));
        if (ec)
        {
            LOG_ERROR("{} timer error {}", id_, ec.message());
            co_return;
        }
    }
    timer_->cancel(ec);
    timer_.reset();
    co_await login(ec);
    if (ec)
    {
        LOG_ERROR("{} login failed {}", id_, ec.message());
        co_return;
    }
    LOG_INFO("{} login success token {}", id_, token_);
    cotrol_ = std::make_shared<leaf::cotrol_session>(id_, host_, port_, token_, handler_.c, executors.get_executor());
    upload_ = std::make_shared<leaf::upload_session>(id_, host_, port_, token_, handler_.u, executors.get_executor());
    download_ = std::make_shared<leaf::download_session>(id_, host_, port_, token_, handler_.d, executors.get_executor());
    cotrol_->startup();
    upload_->startup();
    download_->startup();
}

void file_transfer_client::startup()
{
    executors.startup();
    ex_ = &executors.get_executor();
    boost::asio::co_spawn(*ex_, [this, self = shared_from_this()]() -> boost::asio::awaitable<void> { co_await loop_coro(); }, boost::asio::detached);
}

void file_transfer_client::shutdown()
{
    std::call_once(shutdown_flag_, [this, self = shared_from_this()]() { ex_->post([this, self]() { safe_shutdown(); }); });
}

void file_transfer_client::safe_shutdown()
{
    LOG_INFO("{}  shutdown", id_);
    if (timer_)
    {
        boost::system::error_code ignore;
        timer_->cancel(ignore);
        timer_.reset();
    }
    if (upload_)
    {
        upload_->shutdown();
        upload_.reset();
    }
    if (download_)
    {
        download_->shutdown();
        download_.reset();
    }
    if (cotrol_)
    {
        cotrol_->shutdown();
        cotrol_.reset();
    }
    LOG_INFO("{} shutdown complete", id_);
}

void file_transfer_client::add_upload_file(const std::string &filename)
{
    ex_->post(
        [this, filename]()
        {
            if (upload_)
            {
                upload_->add_file(filename);
            }
        });
}
void file_transfer_client::add_download_file(const std::string &filename)
{
    ex_->post(
        [this, filename]()
        {
            if (download_)
            {
                download_->add_file(filename);
            }
        });
}
void file_transfer_client::add_upload_files(const std::vector<std::string> &files)
{
    ex_->post(
        [this, files]()
        {
            if (upload_)
            {
                upload_->add_files(files);
            }
        });
}
void file_transfer_client::add_download_files(const std::vector<std::string> &files)
{
    ex_->post(
        [this, files]()
        {
            if (download_)
            {
                download_->add_files(files);
            }
        });
}

void file_transfer_client::create_directory(const std::string &dir)
{
    if (cotrol_)
    {
        cotrol_->create_directory(dir);
    }
}
void file_transfer_client::change_current_dir(const std::string &dir)
{
    if (cotrol_)
    {
        cotrol_->change_current_dir(dir);
    }
}
}    // namespace leaf
