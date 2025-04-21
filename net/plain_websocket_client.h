#ifndef LEAF_NET_PLAIN_WEBSOCKET_CLIENT_H
#define LEAF_NET_PLAIN_WEBSOCKET_CLIENT_H

#include <queue>
#include <atomic>
#include <mutex>
#include <functional>
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/thread/latch.hpp>
#include "net/types.h"
#include "net/websocket_session.h"

namespace leaf
{
class plain_websocket_client : public leaf::websocket_session
{
   public:
    using handshake_handler = std::function<void(const boost::system::error_code&)>;
    using message_handler =
        std::function<void(const std::shared_ptr<std::vector<uint8_t>>&, const boost::system::error_code&)>;

   public:
    plain_websocket_client(std::string id,
                           std::string target,
                           boost::asio::ip::tcp::endpoint ed,
                           boost::asio::io_context& io);
    ~plain_websocket_client();

   public:
    void startup() override;
    void shutdown() override;
    void write(const std::vector<uint8_t>& msg) override;
    void set_read_cb(leaf::websocket_session::read_cb cb) override { read_cb_ = std::move(cb); }
    void set_write_cb(leaf::websocket_session::write_cb cb) override { write_cb_ = std::move(cb); }
    void set_handshake_cb(leaf::websocket_session::handshake_cb cb) override { handshake_cb_ = std::move(cb); }

   private:
    void on_connect(boost::beast::error_code ec);
    void on_handshake(boost::beast::error_code ec);
    void safe_startup();
    void safe_shutdown();
    void connect();
    // read
    void safe_read();
    void do_read();
    void on_read(boost::beast::error_code ec, std::size_t bytes_transferred);
    // write
    void safe_write(const std::vector<uint8_t>& msg);
    void do_write();
    void on_write(boost::beast::error_code ec, std::size_t bytes_transferred);
    void on_error(boost::beast::error_code ec);

   private:
    bool writing_ = false;
    std::atomic<bool> shutdown_{false};
    std::string id_;
    boost::asio::io_context& io_;
    std::string target_;
    bool connected_ = false;
    std::once_flag shutdown_flag_;
    boost::beast::flat_buffer buffer_;
    boost::asio::ip::tcp::endpoint ed_;
    leaf::websocket_session::read_cb read_cb_;
    leaf::websocket_session::write_cb write_cb_;
    leaf::websocket_session::handshake_cb handshake_cb_;
    std::queue<std::vector<uint8_t>> msg_queue_;
    std::shared_ptr<boost::beast::websocket::stream<tcp_stream_limited>> ws_;
};
}    // namespace leaf

#endif
