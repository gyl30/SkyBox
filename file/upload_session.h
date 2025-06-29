#ifndef LEAF_FILE_UPLOAD_SESSION_H
#define LEAF_FILE_UPLOAD_SESSION_H

#include <deque>
#include <boost/asio/experimental/channel.hpp>
#include "file/file.h"
#include "file/event.h"
#include "crypt/blake2b.h"
#include "protocol/message.h"
#include "file/file_context.h"
#include "net/plain_websocket_client.h"

namespace leaf
{
class upload_session : public std::enable_shared_from_this<upload_session>
{
   public:
    upload_session(std::string id, std::string host, std::string port, std::string token, boost::asio::io_context &io);

    ~upload_session();

   public:
    void startup();
    void shutdown();
    void add_file(const file_info &file);
    void add_files(const std::vector<file_info> &files);

   private:
    boost::asio::awaitable<void> loop();
    boost::asio::awaitable<void> write(const std::vector<uint8_t> &data, boost::system::error_code &ec);
    boost::asio::awaitable<void> shutdown_coro();
    boost::asio::awaitable<void> delay(int second);
    boost::asio::awaitable<void> send_upload_file_request(const file_info &file, boost::beast::error_code &ec);
    boost::asio::awaitable<void> wait_upload_file_response(boost::beast::error_code &ec);
    boost::asio::awaitable<void> send_ack(boost::beast::error_code &ec);
    boost::asio::awaitable<void> send_file_data(const file_info &file, boost::beast::error_code &ec);
    boost::asio::awaitable<void> send_file_done(boost::beast::error_code &ec);
    boost::asio::awaitable<void> wait_file_done(boost::beast::error_code &ec);
    boost::asio::awaitable<void> send_keepalive(boost::beast::error_code &ec);
    boost::asio::awaitable<void> login(boost::beast::error_code &);

   private:
    void padding_file_event();
    void safe_add_file(const file_info &file);
    void safe_add_files(const std::vector<file_info> &files);

   private:
    uint32_t seq_ = 0;
    std::string id_;
    std::string host_;
    std::string port_;
    std::string token_;
    boost::asio::io_context &io_;
    std::deque<file_info> padding_files_;
    std::shared_ptr<leaf::plain_websocket_client> ws_client_;
};
}    // namespace leaf

#endif
