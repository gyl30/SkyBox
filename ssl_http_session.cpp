#include "ssl_http_session.h"
#include "ssl_websocket_session.h"

namespace leaf
{

ssl_http_session::ssl_http_session(boost::beast::tcp_stream&& stream,
                                   boost::asio::ssl::context& ctx,
                                   boost::beast::flat_buffer&& buffer)
    : buffer_(std::move(buffer)), stream_(std::move(stream), ctx)
{
}

void ssl_http_session::startup()
{
    boost::asio::dispatch(stream_.get_executor(),
                          boost::beast::bind_front_handler(&ssl_http_session::safe_startup, shared_from_this()));
}

void ssl_http_session::safe_startup()
{
    boost::beast::get_lowest_layer(stream_).expires_after(std::chrono::seconds(30));

    stream_.async_handshake(boost::asio::ssl::stream_base::server,
                            buffer_.data(),
                            boost::beast::bind_front_handler(&ssl_http_session::on_handshake, shared_from_this()));
}

void ssl_http_session::shutdown()
{
    boost::asio::dispatch(stream_.get_executor(),
                          boost::beast::bind_front_handler(&ssl_http_session::safe_shutdown, shared_from_this()));
}
void ssl_http_session::safe_shutdown()
{
    boost::system::error_code ec;
    ec = stream_.next_layer().socket().close(ec);
}

void ssl_http_session::on_handshake(boost::beast::error_code ec, std::size_t bytes_used)
{
    if (ec)
    {
        return shutdown();
    }

    buffer_.consume(bytes_used);

    do_read();
}
void ssl_http_session::do_read()
{
    boost::asio::dispatch(stream_.get_executor(),
                          boost::beast::bind_front_handler(&ssl_http_session::safe_read, shared_from_this()));
}

void ssl_http_session::safe_read()
{
    parser_.emplace();

    parser_->body_limit(10000);

    boost::beast::get_lowest_layer(stream_).expires_after(std::chrono::seconds(30));

    boost::beast::http::async_read(
        stream_, buffer_, *parser_, boost::beast::bind_front_handler(&ssl_http_session::on_read, shared_from_this()));
}
void ssl_http_session::on_read(boost::beast::error_code ec, std::size_t bytes_transferred)
{
    boost::ignore_unused(bytes_transferred);

    if (ec)
    {
        return shutdown();
    }

    if (boost::beast::websocket::is_upgrade(parser_->get()))
    {
        boost::beast::get_lowest_layer(stream_).expires_never();
        return std::make_shared<ssl_websocket_session>(std::move(stream_))->startup(parser_->release());
    }

    do_read();
}

}    // namespace leaf
