#ifndef LEAF_SSL_HTTP_SESSION_H
#define LEAF_SSL_HTTP_SESSION_H

#include <memory>
#include <boost/optional.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/beast.hpp>
#include <boost/beast/ssl.hpp>

namespace leaf
{
class ssl_http_session : public std::enable_shared_from_this<ssl_http_session>
{
   public:
    ssl_http_session(boost::beast::tcp_stream&& stream,
                     boost::asio::ssl::context& ctx,
                     boost::beast::flat_buffer&& buffer)
        : buffer_(std::move(buffer)), stream_(std::move(stream), ctx)
    {
    }

    void run()
    {
        boost::beast::get_lowest_layer(stream_).expires_after(std::chrono::seconds(30));

        stream_.async_handshake(boost::asio::ssl::stream_base::server,
                                buffer_.data(),
                                boost::beast::bind_front_handler(&ssl_http_session::on_handshake, shared_from_this()));
    }

    boost::beast::ssl_stream<boost::beast::tcp_stream>& stream() { return stream_; }

    boost::beast::ssl_stream<boost::beast::tcp_stream> release_stream() { return std::move(stream_); }

    void do_eof()
    {
        boost::beast::get_lowest_layer(stream_).expires_after(std::chrono::seconds(30));

        stream_.async_shutdown(boost::beast::bind_front_handler(&ssl_http_session::on_shutdown, shared_from_this()));
    }

   private:
    void on_handshake(boost::beast::error_code ec, std::size_t bytes_used)
    {
        if (ec)
        {
        }

        buffer_.consume(bytes_used);

        do_read();
    }

    void on_shutdown(boost::beast::error_code ec)
    {
        if (ec)
        {
        }
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
            boost::beast::bind_front_handler(&ssl_http_session::on_read, shared_from_this()));
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

            // ssl websocket session
            return;
        }

        do_read();
    }

   private:
    boost::beast::flat_buffer buffer_;
    boost::beast::ssl_stream<boost::beast::tcp_stream> stream_;
    boost::optional<boost::beast::http::request_parser<boost::beast::http::string_body>> parser_;
};

}    // namespace leaf

#endif
