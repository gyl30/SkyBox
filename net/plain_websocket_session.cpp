#include "plain_websocket_session.h"

namespace leaf
{

plain_websocket_session::plain_websocket_session(boost::beast::tcp_stream&& stream) : ws_(std::move(stream)) {}

void plain_websocket_session::startup(const boost::beast::http::request<boost::beast::http::string_body>& req)
{
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
    ws_.async_read(buffer_, boost::beast::bind_front_handler(&plain_websocket_session::on_read, shared_from_this()));
}

void plain_websocket_session::on_read(boost::beast::error_code ec, std::size_t bytes_transferred)
{
    boost::ignore_unused(bytes_transferred);

    if (ec)
    {
        return shutdown();
    }

    ws_.text(ws_.got_text());
    ws_.async_write(buffer_.data(),
                    boost::beast::bind_front_handler(&plain_websocket_session::on_write, shared_from_this()));
}

void plain_websocket_session::on_write(boost::beast::error_code ec, std::size_t bytes_transferred)
{
    boost::ignore_unused(bytes_transferred);

    if (ec)
    {
        return shutdown();
    }
    buffer_.consume(buffer_.size());

    do_read();
}

}    // namespace leaf
