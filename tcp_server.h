#ifndef LEAF_WEBSOCKET_SERVER_H
#define LEAF_WEBSOCKET_SERVER_H

#include <memory>
#include <functional>
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/asio/ssl.hpp>

#include "executors.h"

namespace leaf
{
class tcp_server : public std::enable_shared_from_this<tcp_server>
{
   public:
    using ptr = std::shared_ptr<tcp_server>;

   public:
    struct handle
    {
        std::function<void(boost::beast::error_code)> error;
        std::function<boost::asio::ip::tcp::socket(void)> socket;
        std::function<void(boost::asio::ip::tcp::socket)> accept;
    };

   public:
    tcp_server(handle h, leaf::executors::executor& ex, boost::asio::ip::tcp::endpoint endpoint);
    ~tcp_server();

   public:
    void startup();
    void shutdown();

   private:
    void safe_startup();
    void safe_shutdown();
    void do_accept();
    void on_accept(boost::beast::error_code ec);

   private:
    handle handle_;
    leaf::executors::executor& ex_;
    boost::asio::ip::tcp::endpoint endpoint_;
    boost::asio::ip::tcp::socket socket_{ex_};
    boost::asio::ip::tcp::acceptor acceptor_{ex_};
    boost::asio::ssl::context ssl_ctx_{boost::asio::ssl::context::tls_server};
};

}    // namespace leaf

#endif
