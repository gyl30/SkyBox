#ifndef LEAF_PLAIN_HTTP_SESSION_H
#define LEAF_PLAIN_HTTP_SESSION_H

#include <memory>
#include <boost/optional.hpp>
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include "plain_websocket_session.h"

namespace leaf
{

class plain_http_session : public std::enable_shared_from_this<plain_http_session>
{
   public:
    plain_http_session(boost::beast::tcp_stream&& stream, boost::beast::flat_buffer&& buffer)
        : buffer_(std::move(buffer)), stream_(std::move(stream))
    {
    }

    void run() { do_read(); }

    boost::beast::tcp_stream& stream() { return stream_; }

    boost::beast::tcp_stream release_stream() { return std::move(stream_); }

    void do_eof()
    {
        boost::beast::error_code ec;
        ec = stream_.socket().shutdown(boost::asio::ip::tcp::socket::shutdown_send, ec);
    }

    void do_read()
    {
        parser_.emplace();

        parser_->body_limit(10000);

        boost::beast::get_lowest_layer(stream_).expires_after(std::chrono::seconds(30));

        boost::beast::http::async_read(
            stream_,
            buffer_,
            *parser_,
            boost::beast::bind_front_handler(&plain_http_session::on_read, shared_from_this()));
    }

    void on_read(boost::beast::error_code ec, std::size_t bytes_transferred)
    {
        boost::ignore_unused(bytes_transferred);

        if (ec == boost::beast::http::error::end_of_stream)
        {
        }

        if (ec)
        {
        }

        if (boost::beast::websocket::is_upgrade(parser_->get()))
        {
            boost::beast::get_lowest_layer(stream_).expires_never();
            std::make_shared<leaf::plain_websocket_session>(std::move(stream_))->run(parser_->release());
            return;
        }

        do_read();
    }

   private:
    boost::beast::flat_buffer buffer_;
    boost::beast::tcp_stream stream_;
    boost::optional<boost::beast::http::request_parser<boost::beast::http::string_body>> parser_;
};

}    // namespace leaf

#endif
