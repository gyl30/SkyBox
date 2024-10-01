#ifndef LEAF_PLAIN_WEBSOCKET_SESSION_H
#define LEAF_PLAIN_WEBSOCKET_SESSION_H

#include <queue>
#include <memory>
#include <boost/optional.hpp>
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/beast/websocket.hpp>
#include "websocket_handle.h"
#include "websocket_session.h"

namespace leaf
{

class plain_websocket_session : public leaf::websocket_session
{
   public:
    explicit plain_websocket_session(std::string id,
                                     boost::beast::tcp_stream&& stream,
                                     leaf::websocket_handle::ptr handle);

    ~plain_websocket_session() override;

   public:
    void startup(const boost::beast::http::request<boost::beast::http::string_body>& req) override;
    void shutdown() override;
    void write(const std::string& msg) override;

   private:
    void do_accept(const boost::beast::http::request<boost::beast::http::string_body>& req);
    void on_accept(boost::beast::error_code ec);
    void do_read();
    void safe_read();
    void on_read(boost::beast::error_code ec, std::size_t bytes_transferred);
    void safe_write(const std::string& msg);
    void do_write();
    void on_write(boost::beast::error_code ec, std::size_t bytes_transferred);
    void safe_shutdown();

   private:
    std::string id_;
    std::shared_ptr<void> self_;
    leaf::websocket_handle::ptr h_;
    boost::beast::flat_buffer buffer_;
    std::queue<std::string> msg_queue_;
    boost::beast::websocket::stream<boost::beast::tcp_stream> ws_;
};

}    // namespace leaf

#endif
