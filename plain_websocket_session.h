#ifndef LEAF_PLAIN_WEBSOCKET_SESSION_H
#define LEAF_PLAIN_WEBSOCKET_SESSION_H

#include <memory>
#include <boost/optional.hpp>
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/beast/websocket.hpp>

namespace leaf
{

class plain_websocket_session : public std::enable_shared_from_this<plain_websocket_session>
{
   public:
    explicit plain_websocket_session(boost::beast::tcp_stream&& stream) : ws_(std::move(stream)) {}
    boost::beast::websocket::stream<boost::beast::tcp_stream>& ws() { return ws_; }

   public:
    void run(const boost::beast::http::request<boost::beast::http::string_body>& req) { do_accept(req); }

   private:
    void do_accept(const boost::beast::http::request<boost::beast::http::string_body>& req)
    {
        ws_.set_option(boost::beast::websocket::stream_base::timeout::suggested(boost::beast::role_type::server));

        ws_.set_option(boost::beast::websocket::stream_base::decorator(
            [](boost::beast::websocket::response_type& res)
            { res.set(boost::beast::http::field::server, "leaf/ws"); }));

        ws_.async_accept(req,
                         boost::beast::bind_front_handler(&plain_websocket_session::on_accept, shared_from_this()));
    }

   private:
    void on_accept(boost::beast::error_code ec)
    {
        if (ec)
        {
        }

        do_read();
    }

    void do_read()
    {
        ws_.async_read(buffer_,
                       boost::beast::bind_front_handler(&plain_websocket_session::on_read, shared_from_this()));
    }

    void on_read(boost::beast::error_code ec, std::size_t bytes_transferred)
    {
        boost::ignore_unused(bytes_transferred);

        if (ec == boost::beast::websocket::error::closed)
        {
            return;
        }

        if (ec)
        {
        }

        ws_.text(ws_.got_text());
        ws_.async_write(buffer_.data(),
                        boost::beast::bind_front_handler(&plain_websocket_session::on_write, shared_from_this()));
    }

    void on_write(boost::beast::error_code ec, std::size_t bytes_transferred)
    {
        boost::ignore_unused(bytes_transferred);

        if (ec)
        {
        }
        buffer_.consume(buffer_.size());

        do_read();
    }

   private:
    boost::beast::flat_buffer buffer_;
    boost::beast::websocket::stream<boost::beast::tcp_stream> ws_;
};

}    // namespace leaf

#endif
