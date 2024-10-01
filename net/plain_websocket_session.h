#ifndef LEAF_PLAIN_WEBSOCKET_SESSION_H
#define LEAF_PLAIN_WEBSOCKET_SESSION_H

#include <memory>
#include <boost/optional.hpp>
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/beast/websocket.hpp>
#include "websocket_handle.h"

namespace leaf
{

class plain_websocket_session : public std::enable_shared_from_this<plain_websocket_session>
{
   public:
    explicit plain_websocket_session(boost::beast::tcp_stream&& stream, leaf::websocket_handle::ptr handle);

   public:
    void startup(const boost::beast::http::request<boost::beast::http::string_body>& req);
    void shutdown();
    void write(const std::string& msg);

   private:
    void do_accept(const boost::beast::http::request<boost::beast::http::string_body>& req);
    void on_accept(boost::beast::error_code ec);
    void do_read();
    void safe_read();
    void on_read(boost::beast::error_code ec, std::size_t bytes_transferred);
    void on_write(boost::beast::error_code ec, std::size_t bytes_transferred);
    void safe_shutdown();

   private:
    leaf::websocket_handle::ptr h_;
    boost::beast::flat_buffer buffer_;
    boost::beast::websocket::stream<boost::beast::tcp_stream> ws_;
};

}    // namespace leaf

#endif
