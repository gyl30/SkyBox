#ifndef LEAF_SSL_HTTP_SESSION_H
#define LEAF_SSL_HTTP_SESSION_H

#include <boost/optional.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/beast.hpp>
#include <boost/beast/ssl.hpp>
#include "http_handle.h"
#include "http_session.h"

namespace leaf
{
class ssl_http_session : public http_session
{
   public:
    ssl_http_session(std::string id,
                     boost::beast::tcp_stream&& stream,
                     boost::asio::ssl::context& ctx,
                     boost::beast::flat_buffer&& buffer,
                     leaf::http_handle::ptr handle);

    ~ssl_http_session() override;

   public:
    void startup() override;

    void shutdown() override;

    void write(const http_response_ptr& ptr) override;

   private:
    void safe_startup();
    void safe_shutdown();
    void safe_write(const http_response_ptr& ptr);
    void on_write(bool keep_alive, boost::beast::error_code ec, std::size_t bytes_transferred);
    void on_handshake(boost::beast::error_code ec, std::size_t bytes_used);
    void do_read();
    void safe_read();
    void on_read(boost::beast::error_code ec, std::size_t bytes_transferred);

   private:
    std::string id_;
    std::shared_ptr<void> self_;
    leaf::http_handle::ptr handle_;
    boost::beast::flat_buffer buffer_;
    boost::beast::ssl_stream<boost::beast::tcp_stream> stream_;
    boost::optional<boost::beast::http::request_parser<boost::beast::http::string_body>> parser_;
};

}    // namespace leaf

#endif
