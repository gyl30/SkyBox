#ifndef LEAF_FILE_DOWNLOAD_FILE_HANDLE_H
#define LEAF_FILE_DOWNLOAD_FILE_HANDLE_H

#include <queue>
#include <mutex>
#include <boost/asio/experimental/channel.hpp>
#include "file/file.h"
#include "protocol/message.h"
#include "crypt/blake2b.h"
#include "file/file_context.h"
#include "net/websocket_handle.h"

namespace leaf
{

class download_file_handle : public websocket_handle
{
    struct download_context
    {
        leaf::file_info::ptr file;
        leaf::download_file_request request;
    };

   public:
    explicit download_file_handle(const boost::asio::any_io_executor& io, std::string id, leaf::websocket_session::ptr& session);
    ~download_file_handle() override;

   public:
    void startup() override;
    void shutdown() override;
    std::string type() const override { return "download"; }

   private:
    boost::asio::awaitable<void> recv_coro();
    boost::asio::awaitable<void> write_coro();
    boost::asio::awaitable<void> shutdown_coro();
    boost::asio::awaitable<void> wait_login(boost::beast::error_code& ec);
    boost::asio::awaitable<void> on_keepalive(boost::beast::error_code& ec);
    boost::asio::awaitable<void> error_message(uint32_t id, int32_t error_code);
    boost::asio::awaitable<leaf::download_file_handle::download_context> wait_download_file_request(boost::beast::error_code& ec);
    boost::asio::awaitable<void> send_ack(boost::beast::error_code& ec);
    boost::asio::awaitable<void> send_file_data(leaf::download_file_handle::download_context& ctx, boost::beast::error_code& ec);
    boost::asio::awaitable<void> send_file_done(boost::beast::error_code& ec);

   private:
    std::string id_;
    std::string token_;
    std::once_flag shutdown_flag_;
    leaf::websocket_session::ptr session_;
    const boost::asio::any_io_executor& io_;
    std::queue<leaf::file_info::ptr> padding_files_;
    boost::asio::experimental::channel<void(boost::system::error_code, std::vector<uint8_t>)> channel_{io_, 1024};
};

}    // namespace leaf
#endif
