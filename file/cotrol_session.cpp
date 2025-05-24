#include <utility>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/asio/redirect_error.hpp>

#include "log/log.h"
#include "protocol/codec.h"
#include "protocol/message.h"
#include "file/cotrol_session.h"

namespace leaf
{
cotrol_session::cotrol_session(
    std::string id, std::string host, std::string port, std::string token, leaf::cotrol_handle handler, boost::asio::io_context& io)
    : id_(std::move(id)), host_(std::move(host)), port_(std::move(port)), token_(std::move(token)), handler_(std::move(handler)), io_(io)
{
}

cotrol_session::~cotrol_session() = default;

void cotrol_session::startup()
{
    LOG_INFO("{} startup", id_);
    ws_client_ = std::make_shared<leaf::plain_websocket_client>(id_, host_, port_, "/leaf/ws/cotrol", io_);

    boost::asio::co_spawn(io_, [this, self = shared_from_this()]() -> boost::asio::awaitable<void> { co_await recv_coro(); }, boost::asio::detached);

    boost::asio::co_spawn(io_, [this, self = shared_from_this()]() -> boost::asio::awaitable<void> { co_await write_coro(); }, boost::asio::detached);

    boost::asio::co_spawn(io_, [this, self = shared_from_this()]() -> boost::asio::awaitable<void> { co_await timer_coro(); }, boost::asio::detached);
}

boost::asio::awaitable<void> cotrol_session::recv_coro()
{
    LOG_INFO("{} recv coro startup", id_);
    boost::beast::error_code ec;
    co_await ws_client_->handshake(ec);
    if (ec)
    {
        LOG_ERROR("{} recv coro handshake error {}", id_, ec.message());
        co_return;
    }
    LOG_INFO("{} connect ws client will login use token {}", id_, token_);
    leaf::login_token lt;
    lt.id = 0x01;
    lt.token = token_;
    auto bytes = leaf::serialize_login_token(lt);
    co_await channel_.async_send(boost::system::error_code{}, bytes, boost::asio::redirect_error(boost::asio::use_awaitable, ec));
    if (ec)
    {
        co_return;
    }
    boost::beast::flat_buffer buffer;
    while (true)
    {
        co_await ws_client_->read(ec, buffer);
        if (ec)
        {
            LOG_ERROR("{} recv coro error {}", id_, ec.message());
            break;
        }

        auto message = boost::beast::buffers_to_string(buffer.data());
        auto type = leaf::get_message_type(message);
        if (type != leaf::message_type::files_response)
        {
            ec = boost::system::errc::make_error_code(boost::system::errc::protocol_error);
            break;
        }
        auto files = leaf::deserialize_files_response(std::vector<uint8_t>(message.begin(), message.end()));
        if (!files.has_value())
        {
            break;
        }

        LOG_INFO("{} files response {} file size {}", id_, files->token, files->files.size());
        leaf::notify_event e;
        e.method = "files";
        e.data = files->files;
        handler_.notify(e);
        buffer.consume(buffer.size());
    }
    LOG_INFO("{} recv coro shutdown", id_);
}

boost::asio::awaitable<void> cotrol_session::timer_coro()
{
    LOG_INFO("{} timer coro startup", id_);
    boost::system::error_code ec;
    while (true)
    {
        boost::asio::steady_timer timer(io_, std::chrono::seconds(5));
        co_await timer.async_wait(boost::asio::redirect_error(boost::asio::use_awaitable, ec));
        if (ec)
        {
            LOG_ERROR("{} timer coro error {}", id_, ec.message());
            break;
        }
        leaf::files_request req;
        req.token = token_;
        req.dir = current_dir_;
        auto bytes = leaf::serialize_files_request(req);
        co_await channel_.async_send(ec, bytes, boost::asio::redirect_error(boost::asio::use_awaitable, ec));
        if (ec)
        {
            LOG_ERROR("{} timer coro error {}", id_, ec.message());
            break;
        }
    }
    LOG_INFO("{} timer coro shutdown", id_);
}
boost::asio::awaitable<void> cotrol_session::write_coro()
{
    LOG_INFO("{} write coro startup", id_);
    while (true)
    {
        boost::system::error_code ec;
        auto bytes = co_await channel_.async_receive(boost::asio::redirect_error(boost::asio::use_awaitable, ec));
        if (ec)
        {
            LOG_ERROR("{} write coro error {}", id_, ec.message());
            co_return;
        }
        co_await ws_client_->write(ec, bytes.data(), bytes.size());
        if (ec)
        {
            LOG_ERROR("{} write coro error {}", id_, ec.message());
        }
    }
    LOG_INFO("{} write coro shutdown", id_);
}

void cotrol_session::shutdown()
{
    boost::asio::co_spawn(
        io_, [this, self = shared_from_this()]() -> boost::asio::awaitable<void> { co_await shutdown_coro(); }, boost::asio::detached);
}

boost::asio::awaitable<void> cotrol_session::shutdown_coro()
{
    if (ws_client_)
    {
        channel_.close();
        ws_client_->close();
        ws_client_.reset();
    }
    LOG_INFO("{} shutdown", id_);
    co_return;
}

boost::asio::awaitable<void> cotrol_session::create_directory_coro(const std::string& dir)
{
    leaf::create_dir cd;
    cd.dir = dir;
    cd.token = token_;
    auto bytes = leaf::serialize_create_dir(cd);
    boost::system::error_code ec;
    co_await channel_.async_send(boost::system::error_code{}, bytes, boost::asio::redirect_error(boost::asio::use_awaitable, ec));
}
void cotrol_session::create_directory(const std::string& dir)
{
    boost::asio::co_spawn(
        io_,
        [this, self = shared_from_this(), dir = dir]() -> boost::asio::awaitable<void> { co_await create_directory_coro(dir); },
        boost::asio::detached);
}

void cotrol_session::change_current_dir(const std::string& dir) { current_dir_ = dir; }

}    // namespace leaf
