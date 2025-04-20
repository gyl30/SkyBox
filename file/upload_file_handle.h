#ifndef LEAF_FILE_UPLOAD_FILE_HANDLE_H
#define LEAF_FILE_UPLOAD_FILE_HANDLE_H

#include <mutex>
#include "file/file.h"
#include "protocol/message.h"
#include "crypt/blake2b.h"
#include "file/file_context.h"
#include "net/websocket_handle.h"

namespace leaf
{

class upload_file_handle : public websocket_handle
{
   public:
    explicit upload_file_handle(std::string id, leaf::websocket_session::ptr& session);
    ~upload_file_handle() override;

   public:
    void startup() override;
    void shutdown() override;
    std::string type() const override { return "upload"; }

   private:
    void safe_shutdown();
    void on_read(boost::beast::error_code ec, const std::vector<uint8_t>& bytes);
    void on_write(boost::beast::error_code ec, std::size_t bytes_transferred);
    void on_login(const std::optional<leaf::login_token>& l);
    void on_keepalive(const std::optional<leaf::keepalive>& k);
    void on_upload_file_request(const std::optional<leaf::upload_file_request>& req);
    void on_file_data(const std::optional<leaf::file_data>& d);
    void on_ack(const std::optional<leaf::ack>& a);
    void error_message(uint32_t id, int32_t error_code);
    void reset_state();

   private:
    enum upload_state : uint8_t
    {
        wait_login,
        wait_upload_request,
        wait_file_data,
        wait_ack,
    };
    std::string id_;
    std::string user_;
    std::string token_;
    upload_state state_;
    std::vector<uint8_t> key_;
    leaf::file_context::ptr file_;
    std::once_flag shutdown_flag_;
    std::shared_ptr<leaf::blake2b> hash_;
    std::shared_ptr<leaf::writer> writer_;
    leaf::websocket_session::ptr session_;
};

}    // namespace leaf
#endif
