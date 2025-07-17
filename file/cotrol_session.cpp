#include <utility>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/asio/redirect_error.hpp>

#include "log/log.h"
#include "protocol/codec.h"
#include "protocol/message.h"
#include "file/event_manager.h"
#include "file/cotrol_session.h"

namespace leaf
{
cotrol_session::cotrol_session(std::string id, std::string host, std::string port, std::string token, boost::asio::io_context& io)
    : id_(std::move(id)), host_(std::move(host)), port_(std::move(port)), token_(std::move(token)), io_(io)
{
    LOG_INFO("{} created", id_);
}

cotrol_session::~cotrol_session() { LOG_INFO("{} destroyed", id_); }

void cotrol_session::startup()
{
    LOG_INFO("{} startup host {} port {} token {}", id_, host_, port_, token_);
    ws_client_ = std::make_shared<leaf::plain_websocket_client>(id_, host_, port_, "/leaf/ws/cotrol", io_);

    boost::asio::co_spawn(io_, [this, self = shared_from_this()]() -> boost::asio::awaitable<void> { co_await loop(); }, boost::asio::detached);
}

boost::asio::awaitable<void> cotrol_session::loop()
{
    auto self = shared_from_this();
    LOG_INFO("{} recv startup", id_);
    boost::beast::error_code ec;
    LOG_INFO("{} connect ws client {}:{} token {}", id_, host_, port_, token_);
    co_await ws_client_->handshake(ec);
    if (ec)
    {
        LOG_ERROR("{} handshake error {}", id_, ec.message());
        co_return;
    }
    LOG_INFO("{} connect ws client success {}:{} token {}", id_, host_, port_, token_);
    LOG_INFO("{} login start", id_);
    co_await login(ec);
    if (ec)
    {
        LOG_ERROR("{} login error {}", id_, ec.message());
        co_return;
    }
    LOG_INFO("{} login success", id_);
    timer_ = std::make_shared<boost::asio::steady_timer>(co_await boost::asio::this_coro::executor);
    while (true)
    {
        timer_->expires_after(std::chrono::seconds(5));
        co_await timer_->async_wait(boost::asio::redirect_error(boost::asio::use_awaitable, ec));
        if (ec == boost::asio::error::operation_aborted && !create_dirs_.empty())
        {
            ec = {};
            for (const auto& cd : create_dirs_)
            {
                co_await create_directory_coro(cd);
            }
            continue;
        }
        if (ec)
        {
            LOG_ERROR("{} timer wait error {}", id_, ec.message());
            break;
        }
        co_await send_files_request(ec);
        if (ec)
        {
            LOG_ERROR("{} send files request error {}", id_, ec.message());
            break;
        }
        co_await wait_files_response(ec);
        if (ec)
        {
            LOG_ERROR("{} wait files response error {}", id_, ec.message());
            break;
        }
    }
    LOG_INFO("{} timer shutdown", id_);
    LOG_INFO("{} recv shutdown", id_);
}

boost::asio::awaitable<void> cotrol_session::wait_files_response(boost::beast::error_code& ec)
{
    boost::beast::flat_buffer buffer;
    co_await ws_client_->read(ec, buffer);
    if (ec)
    {
        co_return;
    }

    auto message = boost::beast::buffers_to_string(buffer.data());
    auto type = leaf::get_message_type(message);
    if (type != leaf::message_type::files_response)
    {
        LOG_ERROR("{} files response message type error {}", id_, leaf::message_type_to_string(type));
        ec = boost::system::errc::make_error_code(boost::system::errc::protocol_error);
        co_return;
    }
    auto files = leaf::deserialize_files_response(std::vector<uint8_t>(message.begin(), message.end()));
    if (!files.has_value())
    {
        LOG_ERROR("{} files response deserialize error", id_);
        co_return;
    }

    LOG_INFO("{} files response {} file size {}", id_, files->token, files->files.size());
    leaf::notify_event e;
    e.method = "files";
    e.data = files->files;
    leaf::event_manager::instance().post("notify", e);
    buffer.consume(buffer.size());
}
boost::asio::awaitable<void> cotrol_session::login(boost::beast::error_code& ec)
{
    leaf::login_token lt;
    lt.id = 0x01;
    lt.token = token_;
    co_await write(leaf::serialize_login_token(lt), ec);
    if (ec)
    {
        co_return;
    }
    boost::beast::flat_buffer buffer;
    co_await ws_client_->read(ec, buffer);
    if (ec)
    {
        co_return;
    }
    auto message = boost::beast::buffers_to_string(buffer.data());
    auto type = leaf::get_message_type(message);
    std::vector<uint8_t> bytes(message.begin(), message.end());
    if (type != leaf::message_type::login)
    {
        LOG_ERROR("{} login message type error {}", id_, leaf::message_type_to_string(type));
        ec = boost::system::errc::make_error_code(boost::system::errc::protocol_error);
        co_return;
    }
    auto login = leaf::deserialize_login_token(bytes);
    if (!login.has_value())
    {
        LOG_ERROR("{} login message deserialize error", id_);
        ec = boost::system::errc::make_error_code(boost::system::errc::protocol_error);
        co_return;
    }
}

boost::asio::awaitable<void> cotrol_session::send_files_request(boost::beast::error_code& ec)
{
    leaf::files_request req;
    req.token = token_;
    req.dir = current_dir_;
    co_await write(leaf::serialize_files_request(req), ec);
}

boost::asio::awaitable<void> cotrol_session::write(const std::vector<uint8_t>& data, boost::beast::error_code& ec)
{
    co_await ws_client_->write(ec, data.data(), data.size());
}

void cotrol_session::shutdown()
{
    boost::asio::co_spawn(
        io_, [this, self = shared_from_this()]() -> boost::asio::awaitable<void> { co_await shutdown_coro(); }, boost::asio::detached);
}

boost::asio::awaitable<void> cotrol_session::shutdown_coro()
{
    if (timer_)
    {
        timer_->cancel();
        timer_.reset();
    }

    if (ws_client_)
    {
        ws_client_->close();
        ws_client_.reset();
    }
    LOG_INFO("{} shutdown", id_);
    co_return;
}

boost::asio::awaitable<void> cotrol_session::create_directory_coro(const leaf::create_dir& cd)
{
    boost::beast::error_code ec;
    leaf::notify_event e;
    e.method = "create_directory";

    co_await write(leaf::serialize_create_dir(cd), ec);
    if (ec)
    {
        e.error = ec.message();
        leaf::event_manager::instance().post("notify", e);
        co_return;
    }
    boost::beast::flat_buffer buffer;
    co_await ws_client_->read(ec, buffer);
    if (ec)
    {
        e.error = ec.message();
        leaf::event_manager::instance().post("notify", e);
        co_return;
    }

    auto message = boost::beast::buffers_to_string(buffer.data());
    auto type = leaf::get_message_type(message);
    if (type != leaf::message_type::dir)
    {
        LOG_ERROR("{} create_directory response message type error {}", id_, leaf::message_type_to_string(type));
        ec = boost::system::errc::make_error_code(boost::system::errc::protocol_error);
        e.error = ec.message();
        leaf::event_manager::instance().post("notify", e);
        co_return;
    }
    auto dir_res = leaf::deserialize_create_dir(std::vector<uint8_t>(message.begin(), message.end()));
    if (!dir_res.has_value())
    {
        LOG_ERROR("{} create_directory message deserialize error", id_);
        ec = boost::system::errc::make_error_code(boost::system::errc::protocol_error);
        e.error = ec.message();
        leaf::event_manager::instance().post("notify", e);
        co_return;
    }
    if (dir_res->dir != cd.dir)
    {
        LOG_ERROR("{} create_directory response token {} dir {} not match request dir {}", id_, token_, dir_res->dir, cd.dir);
        e.error = "response dir not match request dir";
        leaf::event_manager::instance().post("notify", e);
    }
    e.data = dir_res.value();
    LOG_INFO("{} create_directory successful token {} dir {}", id_, token_, dir_res->dir);
}
void cotrol_session::create_directory(const leaf::create_dir& cd)
{
    boost::asio::post(io_,
                      [this, self = shared_from_this(), cd = cd]()
                      {
                          create_dirs_.push_back(cd);
                          timer_->cancel();
                      });
}

void cotrol_session::change_current_dir(const std::string& dir)
{
    boost::asio::post(io_, [this, self = shared_from_this(), dir = dir]() { current_dir_ = dir; });
}

}    // namespace leaf
