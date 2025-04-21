#include <utility>
#include "log/log.h"
#include "net/socket.h"
#include "net/buffer.h"
#include "net/plain_websocket_client.h"

namespace leaf
{

plain_websocket_client::plain_websocket_client(std::string id,
                                               std::string target,
                                               boost::asio::ip::tcp::endpoint ed,
                                               boost::asio::io_context& io)
    : id_(std::move(id)), io_(io), target_(std::move(target)), ed_(std::move(ed))
{
    LOG_INFO("create {}", id_);
}

plain_websocket_client::~plain_websocket_client()
{
    //
    LOG_INFO("destroy {}", id_);
}

void plain_websocket_client::startup()
{
    boost::asio::dispatch(io_, boost::beast::bind_front_handler(&plain_websocket_client::safe_startup, this));
}

void plain_websocket_client::safe_startup()
{
    LOG_INFO("startup {}", id_);
    connect();
}

void plain_websocket_client::connect()
{
    ws_ = std::make_shared<boost::beast::websocket::stream<tcp_stream_limited>>(io_);
    auto& l = boost::beast::get_lowest_layer(*ws_);
    l.async_connect(ed_, boost::beast::bind_front_handler(&plain_websocket_client::on_connect, this));
}

void plain_websocket_client::on_connect(boost::beast::error_code ec)
{
    if (ec)
    {
        LOG_ERROR("{} connect to {} failed {}", id_, leaf::get_endpoint_address(ed_), ec.to_string());
        on_error(ec);
        return;
    }
    LOG_INFO("{} connect to {}", id_, leaf::get_endpoint_address(ed_));
    std::string host = leaf::get_endpoint_address(ed_);
    boost::beast::get_lowest_layer(*ws_).expires_after(std::chrono::seconds(3));
    boost::beast::get_lowest_layer(*ws_).expires_never();

    auto user_agent = [](auto& req) { req.set(boost::beast::http::field::user_agent, "leaf/ws"); };

    ws_->binary(true);
    ws_->set_option(boost::beast::websocket::stream_base::timeout::suggested(boost::beast::role_type::client));
    ws_->set_option(boost::beast::websocket::stream_base::decorator(user_agent));
    ws_->async_handshake(host, target_, boost::beast::bind_front_handler(&plain_websocket_client::on_handshake, this));
}

void plain_websocket_client::on_handshake(boost::beast::error_code ec)
{
    if (handshake_cb_)
    {
        handshake_cb_(ec);
    }

    if (ec)
    {
        LOG_ERROR("{} handshake failed {}", id_, ec.message());
        return;
    }

    connected_ = true;
    do_read();
    do_write();
}

void plain_websocket_client::do_read()
{
    boost::asio::dispatch(io_, boost::beast::bind_front_handler(&plain_websocket_client::safe_read, this));
}
void plain_websocket_client::safe_read()
{
    // boost::beast::get_lowest_layer(*ws_).expires_after(std::chrono::seconds(30));
    ws_->async_read(buffer_, boost::beast::bind_front_handler(&plain_websocket_client::on_read, this));
}
void plain_websocket_client::on_read(boost::beast::error_code ec, std::size_t bytes_transferred)
{
    if (ec)
    {
        LOG_ERROR("{} read failed {}", id_, ec.message());
        on_error(ec);
        return;
    }

    LOG_TRACE("{} read message size {}", id_, bytes_transferred);

    auto msg = leaf::buffers_to_vector(buffer_.data());

    buffer_.consume(buffer_.size());

    if (read_cb_)
    {
        read_cb_(ec, msg);
    }
    else
    {
        LOG_ERROR("{} read callback is null", id_);
    }

    do_read();
}

void plain_websocket_client::on_error(boost::beast::error_code ec)
{
    if (ec != boost::asio::error::operation_aborted)
    {
        if (read_cb_)
        {
            read_cb_(ec, {});
        }
    }
}

void plain_websocket_client::shutdown()
{
    // clang-format off
    std::call_once(shutdown_flag_, [this]() { boost::asio::dispatch(io_, [this, self = shared_from_this()]() { safe_shutdown(); }); });
    // clang-format on
}

void plain_websocket_client::safe_shutdown()
{
    read_cb_ = nullptr;
    write_cb_ = nullptr;
    handshake_cb_ = nullptr;
    connected_ = false;
    LOG_INFO("shutdown {}", id_);
    boost::beast::error_code ec;
    ec = ws_->next_layer().socket().close(ec);
    ws_->next_layer().close();
}

void plain_websocket_client::write(const std::vector<uint8_t>& msg)
{
    boost::asio::dispatch(io_, [this, self = shared_from_this(), msg = msg]() { safe_write(msg); });
}

void plain_websocket_client::safe_write(const std::vector<uint8_t>& msg)
{
    assert(!msg.empty());
    msg_queue_.push(msg);
    do_write();
}
void plain_websocket_client::do_write()
{
    if (msg_queue_.empty())
    {
        return;
    }
    if (writing_)
    {
        return;
    }
    if (!connected_)
    {
        return;
    }
    writing_ = true;
    auto& msg = msg_queue_.front();

    ws_->async_write(boost::asio::buffer(msg),
                     boost::beast::bind_front_handler(&plain_websocket_client::on_write, this));
}

void plain_websocket_client::on_write(boost::beast::error_code ec, std::size_t bytes_transferred)
{
    if (ec)
    {
        LOG_ERROR("{} write failed {}", id_, ec.message());
        on_error(ec);
        return;
    }
    if (write_cb_)
    {
        write_cb_(ec, bytes_transferred);
    }
    LOG_TRACE("{} write success {} bytes", id_, bytes_transferred);
    writing_ = false;
    msg_queue_.pop();
    do_write();
}
}    // namespace leaf
