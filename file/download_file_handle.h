#ifndef LEAF_FILE_DOWNLOAD_FILE_HANDLE_H
#define LEAF_FILE_DOWNLOAD_FILE_HANDLE_H

#include <queue>
#include <mutex>
#include "file/file.h"
#include "protocol/codec.h"
#include "protocol/message.h"
#include "crypt/blake2b.h"
#include "file/file_context.h"
#include "net/websocket_handle.h"

namespace leaf
{

class download_file_handle : public websocket_handle
{
   public:
    explicit download_file_handle(std::string id, leaf::websocket_session::ptr& session);
    ~download_file_handle() override;

   public:
    void startup() override;
    void shutdown() override;
    std::string type() const override { return "download"; }

   private:
    void safe_shutdown();
    void on_read(boost::beast::error_code ec, const std::vector<uint8_t>& bytes);
    void on_write(boost::beast::error_code ec, std::size_t bytes_transferred);
    void on_keepalive(const std::optional<leaf::keepalive>& k);
    void on_download_file_request(const std::optional<leaf::download_file_request>& download);
    void on_login(const std::optional<leaf::login_token>& l);
    void on_error_message(const std::optional<leaf::error_message>& e);
    void reset_status();
    void error_message(uint32_t id, int32_t error_code);

   private:
    enum status : uint8_t
    {
        wait_login = 0,
        wait_download_file_request,
        wait_file_data,
    };
    std::string id_;
    std::string token_;
    status status_ = wait_login;
    leaf::file_context::ptr file_;
    std::once_flag shutdown_flag_;
    std::shared_ptr<leaf::reader> reader_;
    std::shared_ptr<leaf::blake2b> hash_;
    leaf::websocket_session::ptr session_;
    std::queue<leaf::file_context::ptr> padding_files_;
};

}    // namespace leaf
#endif
