#ifndef LEAF_FILE_COTROL_FILE_HANDLE_H
#define LEAF_FILE_COTROL_FILE_HANDLE_H

#include <mutex>
#include <boost/asio/experimental/channel.hpp>
#include "protocol/message.h"
#include "net/websocket_handle.h"

namespace leaf
{

class cotrol_file_handle : public websocket_handle
{
   public:
    explicit cotrol_file_handle(const boost::asio::any_io_executor& io, std::string id, leaf::websocket_session::ptr& session);
    ~cotrol_file_handle() override;

   public:
    void startup() override;
    void shutdown() override;
    std::string type() const override { return "cotrol"; }

   private:
    boost::asio::awaitable<void> recv_coro();
    boost::asio::awaitable<void> write_coro();
    boost::asio::awaitable<void> shutdown_coro();
    boost::asio::awaitable<void> wait_login(boost::beast::error_code& ec);
    boost::asio::awaitable<void> keepalive(boost::beast::error_code& ec);
    boost::asio::awaitable<void> error_message(uint32_t id, int32_t error_code);

    boost::asio::awaitable<void> on_files_request(const std::string& message, boost::beast::error_code& ec);
    boost::asio::awaitable<void> on_create_dir(const std::string& message, boost::beast::error_code& ec);

   private:
    std::string id_;
    std::string token_;
    std::once_flag shutdown_flag_;
    leaf::websocket_session::ptr session_;
    const boost::asio::any_io_executor& io_;
    boost::asio::experimental::channel<void(boost::system::error_code, std::vector<uint8_t>)> channel_{io_, 1024};
};

}    // namespace leaf

#endif
