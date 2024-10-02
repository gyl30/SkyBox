#ifndef LEAF_PLAIN_WEBSOCKET_CLIENT_H
#define LEAF_PLAIN_WEBSOCKET_CLIENT_H

#include <queue>
#include <functional>
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include "task_item.h"

namespace leaf
{
class plain_websocket_client : public std::enable_shared_from_this<plain_websocket_client>
{
   public:
    struct handle
    {
        std::function<void(boost::beast::error_code)> on_connect;
        std::function<void(boost::beast::error_code)> on_close;
    };

   public:
    plain_websocket_client(std::string id, boost::asio::ip::tcp::endpoint ed, boost::asio::io_context& io);
    ~plain_websocket_client();

   public:
    void startup();
    void shutdown();
    void add_file(const leaf::file_item::ptr& file);

   private:
    void on_connect(boost::beast::error_code ec);
    void on_handshake(boost::beast::error_code ec);
    void safe_startup();
    void safe_shutdown();
    void safe_add_file(const leaf::file_item::ptr& file);
    void running_task();

   private:
    std::string id_;
    boost::beast::flat_buffer buffer_;
    boost::asio::ip::tcp::endpoint ed_;
    std::queue<leaf::file_item::ptr> padding_files_;
    std::vector<leaf::file_item::ptr> running_files_;
    leaf::plain_websocket_client::handle h_;
    boost::beast::websocket::stream<boost::beast::tcp_stream> ws_;
};
}    // namespace leaf

#endif
