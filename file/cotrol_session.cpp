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
    register_handler();
    boost::asio::co_spawn(io_, [this, self = shared_from_this()]() -> boost::asio::awaitable<void> { co_await loop(); }, boost::asio::detached);
}

boost::asio::awaitable<void> cotrol_session::loop()
{
    auto self = shared_from_this();
    LOG_INFO("{} loop startup", id_);
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

    while (true)
    {
        boost::system::error_code ec;
        auto mp = co_await channel_.async_receive(boost::asio::redirect_error(boost::asio::use_awaitable, ec));
        if (ec)
        {
            LOG_ERROR("{} reciver message from channel error {}", id_, ec.message());
            break;
        }
        LOG_DEBUG("{} ------- {} message ------", id_, mp.type);
        co_await ws_client_->write(ec, mp.message.data(), mp.message.size());
        if (ec)
        {
            LOG_ERROR("{} write message to ws client {}", id_, ec.message());
            break;
        }
        auto it = handlers_.find(mp.type);
        if (it == handlers_.end())
        {
            break;
        }
        LOG_DEBUG("{} ------- {} handle ------", id_, mp.type);
        co_await it->second(ec);
        if (ec)
        {
            LOG_ERROR("{} handle message {} error {}", id_, mp.type, ec.message());
            break;
        }
    }

    LOG_INFO("{} loop shutdown", id_);
}

boost::asio::awaitable<void> cotrol_session::wait_files_response(boost::beast::error_code& ec)
{
    boost::beast::flat_buffer buffer;
    co_await ws_client_->read(ec, buffer);
    if (ec)
    {
        LOG_ERROR("{} send files request error {}", id_, ec.message());
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
    e.data = files.value();
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
    if (ws_client_)
    {
        ws_client_->close();
        ws_client_.reset();
    }
    LOG_INFO("{} shutdown", id_);
    co_return;
}

void cotrol_session::register_handler()
{
    handlers_["files"] = std::bind_front(&cotrol_session::wait_files_response, this);
    handlers_["rename"] = std::bind_front(&cotrol_session::wait_rename_response, this);
    handlers_["create_directory"] = std::bind_front(&cotrol_session::wait_create_response, this);
}
boost::asio::awaitable<void> cotrol_session::wait_create_response(boost::beast::error_code& ec)
{
    leaf::notify_event e;
    e.method = "create_directory";
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
    e.data = dir_res.value();
    LOG_INFO("{} create_directory successful token {} dir {}", id_, token_, dir_res->dir);
}

boost::asio::awaitable<void> cotrol_session::wait_rename_response(boost::beast::error_code& ec)
{
    leaf::notify_event e;
    e.method = "rename";
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
        LOG_ERROR("{} rename response message type error {}", id_, leaf::message_type_to_string(type));
        ec = boost::system::errc::make_error_code(boost::system::errc::protocol_error);
        e.error = ec.message();
        leaf::event_manager::instance().post("notify", e);
        co_return;
    }
    auto rename_response = leaf::deserialize_rename_response(std::vector<uint8_t>(message.begin(), message.end()));
    if (!rename_response.has_value())
    {
        LOG_ERROR("{} rename message deserialize error", id_);
        ec = boost::system::errc::make_error_code(boost::system::errc::protocol_error);
        e.error = ec.message();
        leaf::event_manager::instance().post("notify", e);
        co_return;
    }
    e.data = rename_response.value();
    LOG_INFO("{} rename successful token {} from {} to {} parent {}",
             id_,
             token_,
             rename_response->token,
             rename_response->old_name,
             rename_response->new_name,
             rename_response->parent);
}
void cotrol_session::create_directory(const leaf::create_dir& cd)
{
    message_pack mp;
    mp.type = "create_directory";
    mp.message = leaf::serialize_create_dir(cd);
    push_message(std::move(mp));
}

void cotrol_session::change_current_dir(const std::string& dir)
{
    leaf::files_request req;
    req.token = token_;
    req.dir = dir;
    message_pack mp;
    mp.type = "files";
    mp.message = leaf::serialize_files_request(req);
    push_message(std::move(mp));
}

void cotrol_session::rename(const leaf::rename_request& req)
{
    message_pack mp;
    mp.type = "rename";
    mp.message = leaf::serialize_rename_request(req);
    push_message(std::move(mp));
}

void cotrol_session::push_message(message_pack&& mp)
{
    boost::asio::co_spawn(
        io_,
        [this, self = shared_from_this(), mp = std::move(mp)]() -> boost::asio::awaitable<void>
        {
            boost::system::error_code ec;
            co_await channel_.async_send(boost::system::error_code{}, mp, boost::asio::redirect_error(boost::asio::use_awaitable, ec));
        },
        boost::asio::detached);
}

}    // namespace leaf
