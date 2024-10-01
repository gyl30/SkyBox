#include <utility>

#include "log.h"
#include "plain_websocket_session.h"

namespace leaf
{

plain_websocket_session::plain_websocket_session(std::string id,
                                                 boost::beast::tcp_stream&& stream,
                                                 leaf::websocket_handle::ptr handle)
    : id_(std::move(id)), h_(std::move(handle)), ws_(std::move(stream))
{
    LOG_INFO("create {}", id_);
}

plain_websocket_session::~plain_websocket_session()
{
    //
    LOG_INFO("destroy {}", id_);
}
void plain_websocket_session::startup(const boost::beast::http::request<boost::beast::http::string_body>& req)
{
    LOG_INFO("startup {}", id_);
    do_accept(req);
}

void plain_websocket_session::shutdown()
{
    boost::asio::dispatch(
        ws_.get_executor(),
        boost::beast::bind_front_handler(&plain_websocket_session::safe_shutdown, shared_from_this()));
}
void plain_websocket_session::safe_shutdown()
{
    LOG_INFO("shutdown {}", id_);
    boost::beast::error_code ec;
    ec = ws_.next_layer().socket().close(ec);
}
void plain_websocket_session::do_accept(const boost::beast::http::request<boost::beast::http::string_body>& req)
{
    ws_.set_option(boost::beast::websocket::stream_base::timeout::suggested(boost::beast::role_type::server));

    ws_.set_option(boost::beast::websocket::stream_base::decorator(
        [](boost::beast::websocket::response_type& res) { res.set(boost::beast::http::field::server, "leaf/ws"); }));

    ws_.async_accept(req, boost::beast::bind_front_handler(&plain_websocket_session::on_accept, shared_from_this()));
}

void plain_websocket_session::on_accept(boost::beast::error_code ec)
{
    if (ec)
    {
        LOG_ERROR("{} accept failed {}", id_, ec.message());
        return shutdown();
    }

    do_read();
}

void plain_websocket_session::do_read()
{
    boost::asio::dispatch(ws_.get_executor(),
                          boost::beast::bind_front_handler(&plain_websocket_session::safe_read, shared_from_this()));
}

void plain_websocket_session::safe_read()
{
    boost::beast::get_lowest_layer(ws_).expires_after(std::chrono::seconds(30));
    ws_.async_read(buffer_, boost::beast::bind_front_handler(&plain_websocket_session::on_read, shared_from_this()));
}

void plain_websocket_session::on_read(boost::beast::error_code ec, std::size_t bytes_transferred)
{
    boost::ignore_unused(bytes_transferred);

    if (ec)
    {
        LOG_ERROR("{} read failed {}", id_, ec.message());
        return shutdown();
    }
    std::string msg = boost::beast::buffers_to_string(buffer_.data());

    buffer_.consume(buffer_.size());

    if (ws_.binary())
    {
        h_->on_binary_message(msg);
    }
    else
    {
        h_->on_text_message(msg);
    }
    do_read();
}

void plain_websocket_session::write(const std::string& msg)
{
    boost::asio::dispatch(
        ws_.get_executor(),
        boost::beast::bind_front_handler(&plain_websocket_session::safe_write, shared_from_this(), msg));
}

void plain_websocket_session::safe_write(const std::string& msg)
{
    msg_queue_.push(msg);
    do_write();
}

void plain_websocket_session::do_write()
{
    if (msg_queue_.empty())
    {
        return;
    }

    ws_.async_write(boost::asio::buffer(msg_queue_.front()),
                    boost::beast::bind_front_handler(&plain_websocket_session::on_write, shared_from_this()));
}

void plain_websocket_session::on_write(boost::beast::error_code ec, std::size_t bytes_transferred)
{
    boost::ignore_unused(bytes_transferred);

    if (ec)
    {
        LOG_ERROR("{} write failed {}", id_, ec.message());
        return shutdown();
    }
    msg_queue_.pop();
}

}    // namespace leaf
