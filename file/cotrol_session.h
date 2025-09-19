#ifndef LEAF_FILE_COTROL_SESSION_H
#define LEAF_FILE_COTROL_SESSION_H

#include <memory>
#include <optional>
#include <string_view>
#include <queue>
#include <boost/asio/experimental/awaitable_operators.hpp>
#include <boost/asio/experimental/channel.hpp>
#include "file/event.h"
#include "protocol/codec.h"
#include "net/plain_websocket_client.h"

namespace leaf
{
class cotrol_session : public std::enable_shared_from_this<cotrol_session>
{
   public:
    cotrol_session(std::string id, std::string host, std::string port, std::string token, boost::asio::io_context &io);
    ~cotrol_session();

   public:
    void startup();
    void shutdown();
    void create_directory(const leaf::create_dir &cd);
    void change_current_dir(const std::string &dir);
    void rename(const leaf::rename_request &req);

   private:
    boost::asio::awaitable<void> loop();
    boost::asio::awaitable<void> loop1(boost::beast::error_code &ec);
    boost::asio::awaitable<void> login(boost::beast::error_code &);
    boost::asio::awaitable<void> wait_files_response(boost::beast::error_code &);
    boost::asio::awaitable<void> wait_create_response(boost::beast::error_code &);
    boost::asio::awaitable<void> wait_rename_response(boost::beast::error_code &);
    boost::asio::awaitable<void> shutdown_coro();
    boost::asio::awaitable<void> write(const std::vector<uint8_t> &data, boost::beast::error_code &);
    void register_handler();

   private:
    struct message_pack
    {
        std::string type;
        std::vector<uint8_t> message;
    };

   private:
    void push_message(message_pack &&mp);
    boost::asio::awaitable<void> delay(int second, boost::beast::error_code &ec);

   private:
    std::string id_;
    std::string host_;
    std::string port_;
    std::string token_;
    bool shutdown_ = false;
    boost::asio::io_context &io_;
    std::shared_ptr<boost::asio::steady_timer> timer_;
    std::shared_ptr<leaf::plain_websocket_client> ws_client_;
    std::map<std::string, std::function<boost::asio::awaitable<void>(boost::system::error_code &)>> handlers_;
    boost::asio::experimental::channel<void(boost::system::error_code, message_pack)> channel_{io_, 1024};
};

}    // namespace leaf

#endif
