#ifndef LEAF_FILE_UPLOAD_FILE_HANDLE_H
#define LEAF_FILE_UPLOAD_FILE_HANDLE_H

#include <mutex>
#include "protocol/message.h"
#include "crypt/blake2b.h"
#include "file/file_context.h"
#include "net/websocket_handle.h"

namespace leaf
{

class upload_file_handle : public websocket_handle
{
    struct upload_context
    {
        leaf::file_info file;
        leaf::upload_file_request request;
    };

   public:
    explicit upload_file_handle(const boost::asio::any_io_executor& io, std::string id, leaf::websocket_session::ptr& session);
    ~upload_file_handle() override;

   public:
    void startup() override;
    void shutdown() override;
    std::string type() const override { return "upload"; }

   private:
    boost::asio::awaitable<void> loop();
    boost::asio::awaitable<void> write(const std::vector<uint8_t>& msg, boost::beast::error_code& ec);
    boost::asio::awaitable<void> shutdown_coro();
    boost::asio::awaitable<void> wait_login(boost::beast::error_code& ec);
    boost::asio::awaitable<leaf::keepalive> wait_keepalive(boost::beast::error_code& ec);
    boost::asio::awaitable<leaf::upload_file_handle::upload_context> wait_upload_file_request(boost::beast::error_code& ec);
    boost::asio::awaitable<void> wait_ack(boost::beast::error_code& ec);
    boost::asio::awaitable<void> wait_file_data(leaf::upload_file_handle::upload_context& ctx, boost::beast::error_code& ec);
    boost::asio::awaitable<void> send_file_done(boost::beast::error_code& ec);

   private:
    std::string id_;
    std::string user_;
    std::string token_;
    std::vector<uint8_t> key_;
    std::once_flag shutdown_flag_;
    leaf::websocket_session::ptr session_;
    const boost::asio::any_io_executor& io_;
};

}    // namespace leaf
#endif
