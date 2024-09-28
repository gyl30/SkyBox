#ifndef LEAF_SSL_WEBSOCKET_SESSION_H
#define LEAF_SSL_WEBSOCKET_SESSION_H

#include <memory>
#include <boost/optional.hpp>
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/ssl/ssl_stream.hpp>

namespace leaf
{
class ssl_websocket_session : public std::enable_shared_from_this<ssl_websocket_session>
{
   public:
    explicit ssl_websocket_session(boost::beast::ssl_stream<boost::beast::tcp_stream> stream);
    ~ssl_websocket_session();

   public:
    void startup(const boost::beast::http::request<boost::beast::http::string_body>& req);
    void shutdown();

   private:
    void safe_shutdown();
    void on_accept(boost::beast::error_code ec);
    void do_read();
    void safe_read();
    void on_read(boost::beast::error_code ec, std::size_t bytes_transferred);
    void on_write(boost::beast::error_code ec, std::size_t bytes_transferred);

   private:
    boost::beast::flat_buffer buffer_;
    boost::beast::websocket::stream<boost::beast::ssl_stream<boost::beast::tcp_stream>> ws_;
};

}    // namespace leaf

#endif
