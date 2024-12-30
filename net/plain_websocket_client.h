#ifndef LEAF_NET_PLAIN_WEBSOCKET_CLIENT_H
#define LEAF_NET_PLAIN_WEBSOCKET_CLIENT_H

#include <queue>
#include <functional>
#include <boost/asio.hpp>
#include <boost/beast.hpp>

namespace leaf
{
class plain_websocket_client : public std::enable_shared_from_this<plain_websocket_client>
{
   public:
    using message_handler =
        std::function<void(const std::shared_ptr<std::vector<uint8_t>>&, const boost::system::error_code&)>;

   public:
    plain_websocket_client(std::string id,
                           std::string target,
                           boost::asio::ip::tcp::endpoint ed,
                           boost::asio::io_context& io);
    ~plain_websocket_client();

   public:
    void startup();
    void shutdown();
    void write(std::vector<uint8_t> msg);
    void set_message_handler(const message_handler& handler) { message_handler_ = handler; }

   private:
    void on_connect(boost::beast::error_code ec);
    void on_handshake(boost::beast::error_code ec);
    void safe_startup();
    void safe_shutdown();
    // read
    void safe_read();
    void do_read();
    void on_read(boost::beast::error_code ec, std::size_t bytes_transferred);
    // write
    void safe_write(const std::vector<uint8_t>& msg);
    void do_write();
    void on_write(boost::beast::error_code ec, std::size_t bytes_transferred);
    void on_message(const std::shared_ptr<std::vector<uint8_t>>& msg);
    void on_error(boost::beast::error_code ec);

   private:
    bool writing_ = false;
    std::string id_;
    std::string target_;
    message_handler message_handler_;
    boost::beast::flat_buffer buffer_;
    boost::asio::ip::tcp::endpoint ed_;
    std::queue<std::vector<uint8_t>> msg_queue_;
    std::shared_ptr<boost::asio::steady_timer> timer_;
    boost::beast::websocket::stream<boost::beast::tcp_stream> ws_;
};
}    // namespace leaf

#endif
