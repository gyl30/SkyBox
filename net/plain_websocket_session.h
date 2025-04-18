#ifndef LEAF_NET_PLAIN_WEBSOCKET_SESSION_H
#define LEAF_NET_PLAIN_WEBSOCKET_SESSION_H

#include <queue>
#include <memory>
#include <boost/optional.hpp>
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/beast/websocket.hpp>
#include "net/websocket_handle.h"
#include "net/websocket_session.h"

namespace leaf
{

class plain_websocket_session : public leaf::websocket_session
{
   public:
    explicit plain_websocket_session(std::string id,
                                     boost::beast::tcp_stream&& stream,
                                     boost::beast::http::request<boost::beast::http::string_body> req);
    ~plain_websocket_session() override;

   public:
    void startup() override;
    void shutdown() override;
    void write(const std::vector<uint8_t>& msg) override;
    void set_read_cb(leaf::websocket_session::read_cb cb) override;
    void set_write_cb(leaf::websocket_session::write_cb cb) override;
    void set_handshake_cb(leaf::websocket_session::handshake_cb cb) override;

   private:
    void do_accept(const boost::beast::http::request<boost::beast::http::string_body>& req);
    void on_accept(boost::beast::error_code ec);
    void do_read();
    void safe_read();
    void on_read(boost::beast::error_code ec, std::size_t bytes_transferred);
    void safe_write(const std::vector<uint8_t>& msg);
    void do_write();
    void on_write(boost::beast::error_code ec, std::size_t bytes_transferred);
    void safe_shutdown();

   private:
    std::string id_;
    bool writing_ = false;
    std::shared_ptr<void> self_;
    boost::beast::flat_buffer buffer_;
    leaf::websocket_session::read_cb read_cb_;
    leaf::websocket_session::write_cb write_cb_;
    leaf::websocket_session::handshake_cb handshake_cb_;
    std::queue<std::vector<uint8_t>> msg_queue_;
    boost::beast::websocket::stream<boost::beast::tcp_stream> ws_;
    boost::beast::http::request<boost::beast::http::string_body> req_;
};

}    // namespace leaf

#endif
