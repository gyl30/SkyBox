#include <utility>
#include "log/log.h"
#include "net/buffer.h"
#include "net/ssl_websocket_session.h"

namespace leaf
{

ssl_websocket_session::ssl_websocket_session(std::string id,
                                             boost::beast::ssl_stream<boost::beast::tcp_stream>&& stream,
                                             boost::beast::http::request<boost::beast::http::string_body> req)
    : id_(std::move(id)), req_(std::move(req)), ws_(std::move(stream))
{
    LOG_INFO("create {}", id_);
}

ssl_websocket_session::~ssl_websocket_session() { LOG_INFO("destroy {}", id_); }

void ssl_websocket_session::set_read_cb(leaf::websocket_session::read_cb cb) { read_cb_ = std::move(cb); }

void ssl_websocket_session::set_write_cb(leaf::websocket_session::write_cb cb) { write_cb_ = std::move(cb); }

void ssl_websocket_session::startup()
{
    assert(read_cb_ && write_cb_);
    self_ = shared_from_this();
    LOG_INFO("startup {}", id_);
    ws_.set_option(boost::beast::websocket::stream_base::timeout::suggested(boost::beast::role_type::server));

    ws_.set_option(boost::beast::websocket::stream_base::decorator(
        [](boost::beast::websocket::response_type& res) { res.set(boost::beast::http::field::server, "leaf/ws"); }));

    ws_.async_accept(req_, boost::beast::bind_front_handler(&ssl_websocket_session::on_accept, this));
}

void ssl_websocket_session::on_accept(boost::beast::error_code ec)
{
    if (ec)
    {
        LOG_ERROR("{} accept failed {}", id_, ec.message());
        shutdown();
        return;
    }
    do_read();
}

void ssl_websocket_session::shutdown()
{
    boost::asio::dispatch(ws_.get_executor(),
                          boost::beast::bind_front_handler(&ssl_websocket_session::safe_shutdown, this));
}

void ssl_websocket_session::safe_shutdown()
{
    LOG_INFO("shutdown {}", id_);
    boost::system::error_code ec;
    ec = ws_.next_layer().next_layer().socket().close(ec);
    auto self = self_;
    self_.reset();
    boost::asio::dispatch(ws_.get_executor(), [self]() mutable { self.reset(); });
}

void ssl_websocket_session::do_read()
{
    boost::asio::dispatch(ws_.get_executor(),
                          boost::beast::bind_front_handler(&ssl_websocket_session::safe_read, this));
}

void ssl_websocket_session::safe_read()
{
    boost::beast::get_lowest_layer(ws_).expires_after(std::chrono::seconds(30));
    ws_.async_read(buffer_, boost::beast::bind_front_handler(&ssl_websocket_session::on_read, this));
}

void ssl_websocket_session::on_read(boost::beast::error_code ec, std::size_t bytes_transferred)
{
    boost::ignore_unused(bytes_transferred);

    std::vector<uint8_t> bytes;
    if (ec)
    {
        LOG_ERROR("{} read failed {}", id_, ec.message());
        read_cb_(ec, bytes);
        return;
    }
    buffer_.consume(buffer_.size());

    if (!ws_.binary())
    {
        shutdown();
        return;
    }

    bytes = leaf::buffers_to_vector(buffer_.data());
    read_cb_(ec, bytes);

    do_read();
}
void ssl_websocket_session::write(const std::vector<uint8_t>& msg)
{
    boost::asio::dispatch(ws_.get_executor(),
                          boost::beast::bind_front_handler(&ssl_websocket_session::safe_write, this, msg));
}

void ssl_websocket_session::safe_write(const std::vector<uint8_t>& msg)
{
    msg_queue_.push(msg);
    do_write();
}

void ssl_websocket_session::do_write()
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
    ws_.async_write(boost::asio::buffer(msg), boost::beast::bind_front_handler(&ssl_websocket_session::on_write, this));
}

void ssl_websocket_session::on_write(boost::beast::error_code ec, std::size_t bytes_transferred)
{
    boost::ignore_unused(bytes_transferred);

    write_cb_(ec, bytes_transferred);
    if (ec)
    {
        LOG_ERROR("{} write failed {}", id_, ec.message());
        return;
    }
    writing_ = false;
    msg_queue_.pop();
    do_write();
}

}    // namespace leaf
