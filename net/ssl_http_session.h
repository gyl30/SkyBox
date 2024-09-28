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
                     boost::beast::flat_buffer&& buffer);
    void startup();

    void shutdown();

   private:
    void safe_startup();
    void safe_shutdown();
    void on_handshake(boost::beast::error_code ec, std::size_t bytes_used);
    void do_read();
    void safe_read();
    void on_read(boost::beast::error_code ec, std::size_t bytes_transferred);

   private:
    boost::beast::flat_buffer buffer_;
    boost::beast::ssl_stream<boost::beast::tcp_stream> stream_;
    boost::optional<boost::beast::http::request_parser<boost::beast::http::string_body>> parser_;
};

}    // namespace leaf

#endif
