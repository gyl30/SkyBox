#ifndef LEAF_WEBSOCKET_SESSION_H
#define LEAF_WEBSOCKET_SESSION_H

#include <memory>
#include <string>
#include <queue>
#include <functional>
#include <boost/asio.hpp>
#include <boost/beast.hpp>

namespace leaf
{

class websocket_session : public std::enable_shared_from_this<websocket_session>
{
   public:
    struct handle
    {
        std::function<void(boost::beast::error_code)> read_error;
        std::function<void(boost::beast::error_code)> write_error;
        std::function<void(boost::beast::error_code)> handshake_error;
        std::function<void(const std::string &)> text;
        std::function<void(const std::string &)> binary;
    };

   public:
    websocket_session(leaf::websocket_session::handle h, boost::asio::ip::tcp::socket socket);
    ~websocket_session();

   public:
    void startup();
    void shutdown();
    void write(const std::shared_ptr<std::string> &msg);

   private:
    void safe_startup();
    void safe_shutdown();
    void do_accept();
    void on_accept(boost::beast::error_code ec);
    void do_read();
    void on_read(boost::beast::error_code ec, std::size_t transferred);
    void safe_write(const std::shared_ptr<std::string> &msg);
    void do_write();
    void on_write(boost::beast::error_code ec, std::size_t transferred);

   private:
    handle handle_;
    boost::beast::flat_buffer buffer_;
    std::queue<std::shared_ptr<std::string>> queue_;
    boost::beast::websocket::stream<boost::beast::tcp_stream> ws_;
};

}    // namespace leaf

#endif
