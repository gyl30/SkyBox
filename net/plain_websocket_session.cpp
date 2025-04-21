#include <utility>
#include <boost/asio/buffer.hpp>
#include <boost/beast/core/buffers_range.hpp>
#include "log/log.h"
#include "net/buffer.h"
#include "net/plain_websocket_session.h"

namespace leaf
{

plain_websocket_session::plain_websocket_session(std::string id,
                                                 tcp_stream_limited&& stream,
                                                 boost::beast::http::request<boost::beast::http::string_body> req)
    : id_(std::move(id)), ws_(std::move(stream)), req_(std::move(req))
{
    LOG_INFO("create {}", id_);
}

plain_websocket_session::~plain_websocket_session()
{
    //
    LOG_INFO("destroy {}", id_);
}

void plain_websocket_session::set_read_cb(leaf::websocket_session::read_cb cb) { read_cb_ = std::move(cb); }

void plain_websocket_session::set_write_cb(leaf::websocket_session::write_cb cb) { write_cb_ = std::move(cb); };

void plain_websocket_session::set_handshake_cb(leaf::websocket_session::handshake_cb cb)
{
    handshake_cb_ = std::move(cb);
}

void plain_websocket_session::startup()
{
    assert(read_cb_ && write_cb_);
    ws_.binary(true);
    LOG_INFO("startup {}", id_);
    do_accept(req_);
}

void plain_websocket_session::shutdown()
{
    boost::asio::dispatch(ws_.get_executor(),
                          boost::beast::bind_front_handler(&plain_websocket_session::safe_shutdown, this));
}
void plain_websocket_session::safe_shutdown()
{
    LOG_INFO("shutdown {}", id_);
    read_cb_ = nullptr;
    write_cb_ = nullptr;
    boost::beast::error_code ec;
    ec = ws_.next_layer().socket().close(ec);
    auto self = self_;
    self_.reset();
    boost::asio::dispatch(ws_.get_executor(), [self]() mutable { self.reset(); });
}
void plain_websocket_session::do_accept(const boost::beast::http::request<boost::beast::http::string_body>& req)
{
    ws_.set_option(boost::beast::websocket::stream_base::timeout::suggested(boost::beast::role_type::server));

    ws_.set_option(boost::beast::websocket::stream_base::decorator(
        [](boost::beast::websocket::response_type& res) { res.set(boost::beast::http::field::server, "leaf/ws"); }));

    ws_.async_accept(req, boost::beast::bind_front_handler(&plain_websocket_session::on_accept, this));
}

void plain_websocket_session::on_accept(boost::beast::error_code ec)
{
    if (handshake_cb_)
    {
        handshake_cb_(ec);
    }

    if (ec)
    {
        LOG_ERROR("{} accept failed {}", id_, ec.message());
        shutdown();
        return;
    }

    do_read();
}

void plain_websocket_session::do_read()
{
    boost::asio::dispatch(ws_.get_executor(),
                          boost::beast::bind_front_handler(&plain_websocket_session::safe_read, this));
}

void plain_websocket_session::safe_read()
{
    // boost::beast::get_lowest_layer(ws_).expires_after(std::chrono::seconds(30));
    ws_.async_read(buffer_, boost::beast::bind_front_handler(&plain_websocket_session::on_read, this));
}

void plain_websocket_session::on_read(boost::beast::error_code ec, std::size_t bytes_transferred)
{
    boost::ignore_unused(bytes_transferred);
    std::vector<uint8_t> bytes;
    if (ec)
    {
        LOG_ERROR("{} read failed {}", id_, ec.message());
        if (read_cb_)
        {
            read_cb_(ec, bytes);
        }
        return;
    }
    if (!ws_.got_binary())
    {
        shutdown();
        return;
    }

    LOG_TRACE("{} read message size {}", id_, bytes_transferred);
    bytes = leaf::buffers_to_vector(buffer_.data());
    buffer_.consume(buffer_.size());
    if (read_cb_)
    {
        read_cb_(ec, bytes);
    }
    do_read();
}

void plain_websocket_session::write(const std::vector<uint8_t>& msg)
{
    boost::asio::dispatch(ws_.get_executor(),
                          boost::beast::bind_front_handler(&plain_websocket_session::safe_write, this, msg));
}

void plain_websocket_session::safe_write(const std::vector<uint8_t>& msg)
{
    assert(!msg.empty());
    msg_queue_.push(msg);
    do_write();
}

void plain_websocket_session::do_write()
{
    if (msg_queue_.empty())
    {
        return;
    }
    if (writing_)
    {
        return;
    }
    writing_ = true;
    auto& msg = msg_queue_.front();
    ws_.async_write(boost::asio::buffer(msg),
                    boost::beast::bind_front_handler(&plain_websocket_session::on_write, this));
}

void plain_websocket_session::on_write(boost::beast::error_code ec, std::size_t bytes_transferred)
{
    if (write_cb_)
    {
        write_cb_(ec, bytes_transferred);
    }
    if (ec)
    {
        LOG_ERROR("{} write failed {}", id_, ec.message());
        return;
    }
    LOG_TRACE("{} write success {} bytes", id_, bytes_transferred);
    msg_queue_.pop();
    writing_ = false;
    do_write();
}

}    // namespace leaf
