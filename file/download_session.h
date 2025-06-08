#ifndef LEAF_FILE_DOWNLOAD_SESSION_H
#define LEAF_FILE_DOWNLOAD_SESSION_H

#include <boost/asio/experimental/awaitable_operators.hpp>
#include <boost/asio/experimental/channel.hpp>

#include "file/file.h"
#include "file/event.h"
#include "crypt/blake2b.h"
#include "protocol/message.h"
#include "file/file_context.h"
#include "net/plain_websocket_client.h"

namespace leaf
{
class download_session : public std::enable_shared_from_this<download_session>
{
    struct download_context
    {
        std::shared_ptr<leaf::file_info> file;
        leaf::download_file_response response;
    };

   public:
    download_session(std::string id, std::string host, std::string port, std::string token, boost::asio::io_context &io);

    ~download_session();

   public:
    void startup();
    void shutdown();
    void update();
    void add_file(const std::string &file);
    void add_files(const std::vector<std::string> &files);

   public:
    boost::asio::awaitable<void> login(boost::beast::error_code &);
    boost::asio::awaitable<void> download_coro();
    boost::asio::awaitable<void> write_coro();
    boost::asio::awaitable<void> shutdown_coro();
    boost::asio::awaitable<void> send_keepalive(boost::beast::error_code &ec);
    boost::asio::awaitable<void> download(boost::beast::error_code &ec);
    boost::asio::awaitable<void> send_download_file_request(const std::string &filename, boost::beast::error_code &ec);
    boost::asio::awaitable<leaf::download_session::download_context> wait_download_file_response(boost::beast::error_code &ec);
    boost::asio::awaitable<void> wait_ack(boost::beast::error_code &ec);
    boost::asio::awaitable<void> wait_file_data(leaf::download_session::download_context &ctx, boost::beast::error_code &ec);
    boost::asio::awaitable<void> wait_file_done(boost::beast::error_code &ec);
    boost::asio::awaitable<void> delay(int second);

   private:
    void safe_add_file(const std::string &filename);
    void safe_add_files(const std::vector<std::string> &files);

   private:
    uint32_t seq_ = 0;
    std::string id_;
    std::string host_;
    std::string port_;
    std::string token_;
    boost::asio::io_context &io_;
    std::queue<std::string> padding_files_;
    std::shared_ptr<leaf::plain_websocket_client> ws_client_;
    boost::asio::experimental::channel<void(boost::system::error_code, std::vector<uint8_t>)> channel_{io_, 1024};
};
}    // namespace leaf

#endif
