#include "plain_http_session.h"
#include "plain_websocket_session.h"

namespace leaf
{

plain_http_session::plain_http_session(boost::beast::tcp_stream&& stream, boost::beast::flat_buffer&& buffer)
    : buffer_(std::move(buffer)), stream_(std::move(stream))
{
}

void plain_http_session::startup() { do_read(); }

void plain_http_session::shutdown()
{
    boost::asio::dispatch(stream_.get_executor(),
                          boost::beast::bind_front_handler(&plain_http_session::safe_shutdown, shared_from_this()));
}

void plain_http_session::do_read()
{
    boost::asio::dispatch(stream_.get_executor(),
                          boost::beast::bind_front_handler(&plain_http_session::safe_read, shared_from_this()));
}
void plain_http_session::safe_read()
{
    parser_.emplace();

    parser_->body_limit(10000);

    boost::beast::get_lowest_layer(stream_).expires_after(std::chrono::seconds(30));

    boost::beast::http::async_read(
        stream_, buffer_, *parser_, boost::beast::bind_front_handler(&plain_http_session::on_read, shared_from_this()));
}
void plain_http_session::on_read(boost::beast::error_code ec, std::size_t bytes_transferred)
{
    boost::ignore_unused(bytes_transferred);

    if (ec)
    {
        return shutdown();
    }

    if (boost::beast::websocket::is_upgrade(parser_->get()))
    {
        boost::beast::get_lowest_layer(stream_).expires_never();
        return std::make_shared<leaf::plain_websocket_session>(std::move(stream_))->startup(parser_->release());
    }

    do_read();
}

void plain_http_session::safe_shutdown()
{
    boost::beast::error_code ec;
    ec = stream_.socket().shutdown(boost::asio::ip::tcp::socket::shutdown_send, ec);
}

}    // namespace leaf
