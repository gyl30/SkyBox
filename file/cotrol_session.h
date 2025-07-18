#ifndef LEAF_FILE_COTROL_SESSION_H
#define LEAF_FILE_COTROL_SESSION_H

#include <memory>
#include <optional>
#include <string_view>
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

   private:
    boost::asio::awaitable<void> loop();
    boost::asio::awaitable<void> write(const std::vector<uint8_t> &data, boost::beast::error_code &);
    boost::asio::awaitable<void> create_directory_coro(const leaf::create_dir &cd);
    boost::asio::awaitable<void> shutdown_coro();
    boost::asio::awaitable<void> login(boost::beast::error_code &);
    boost::asio::awaitable<void> wait_files_response(boost::beast::error_code &);
    boost::asio::awaitable<void> send_files_request(boost::beast::error_code &);

   private:
    std::string id_;
    std::string host_;
    std::string port_;
    std::string token_;
    std::string current_dir_;
    boost::asio::io_context &io_;
    std::vector<leaf::create_dir> create_dirs_;
    std::shared_ptr<boost::asio::steady_timer> timer_;
    std::shared_ptr<leaf::plain_websocket_client> ws_client_;
};

}    // namespace leaf

#endif
