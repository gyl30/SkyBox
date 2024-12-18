#ifndef LEAF_NET_SSL_WEBSOCKET_SESSION_H
#define LEAF_NET_SSL_WEBSOCKET_SESSION_H

#include <queue>
#include <memory>
#include <boost/optional.hpp>
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/ssl/ssl_stream.hpp>
#include "net/websocket_handle.h"
#include "net/websocket_session.h"

namespace leaf
{
class ssl_websocket_session : public leaf::websocket_session
{
   public:
    explicit ssl_websocket_session(std::string id,
                                   boost::beast::ssl_stream<boost::beast::tcp_stream> stream,
                                   leaf::websocket_handle::ptr handle);
    ~ssl_websocket_session() override;

   public:
    void startup(const boost::beast::http::request<boost::beast::http::string_body>& req) override;
    void shutdown() override;
    void write(const std::vector<uint8_t>& msg) override;

   private:
    void safe_shutdown();
    void on_accept(boost::beast::error_code ec);
    void do_read();
    void safe_read();
    void on_read(boost::beast::error_code ec, std::size_t bytes_transferred);
    void safe_write(const std::vector<uint8_t>& msg);
    void do_write();
    void on_write(boost::beast::error_code ec, std::size_t bytes_transferred);

   private:
    std::string id_;
    std::shared_ptr<void> self_;
    leaf::websocket_handle::ptr h_;
    boost::beast::flat_buffer buffer_;
    std::queue<std::vector<uint8_t>> msg_queue_;
    boost::beast::websocket::stream<boost::beast::ssl_stream<boost::beast::tcp_stream>> ws_;
};

}    // namespace leaf

#endif
