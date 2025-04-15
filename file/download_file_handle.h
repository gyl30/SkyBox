#ifndef LEAF_FILE_DOWNLOAD_FILE_HANDLE_H
#define LEAF_FILE_DOWNLOAD_FILE_HANDLE_H

#include <queue>
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
    explicit download_file_handle(std::string id);
    ~download_file_handle() override;

   public:
    void startup() override;
    void shutdown() override;
    void update(const leaf::websocket_session::ptr&) override;
    std::string type() const override { return "download"; }
    void on_message(const leaf::websocket_session::ptr& session,
                    const std::shared_ptr<std::vector<uint8_t>>& bytes) override;

   private:
    void on_keepalive(const std::optional<leaf::keepalive>& message);
    void on_download_file_request(const std::optional<leaf::download_file_request>& message);
    void on_login(const std::optional<leaf::login_token>& message);
    void on_error_message(const std::optional<leaf::error_message>& e);

   private:
    enum status : uint8_t
    {
        wait_login = 0,
        wait_download_file_request,
        file_data,
    };
    std::string id_;
    std::string token_;
    status status_ = wait_login;
    leaf::file_context::ptr file_;
    std::shared_ptr<leaf::reader> reader_;
    std::shared_ptr<leaf::blake2b> hash_;
    std::queue<std::vector<uint8_t>> msg_queue_;
    std::queue<leaf::file_context::ptr> padding_files_;
};

}    // namespace leaf
#endif
