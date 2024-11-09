#ifndef LEAF_DOWNLOAD_FILE_HANDLE_H
#define LEAF_DOWNLOAD_FILE_HANDLE_H

#include <queue>
#include "codec.h"
#include "message.h"
#include "blake2b.h"
#include "file_context.h"
#include "websocket_handle.h"

namespace leaf
{

class download_file_handle : public websocket_handle
{
   public:
    explicit download_file_handle(std::string id);
    ~download_file_handle() override;

   public:
    void startup() override;
    void on_message(const leaf::websocket_session::ptr& session,
                    const std::shared_ptr<std::vector<uint8_t>>& bytes) override;
    void shutdown() override;

   private:
    void on_download_file_request(const leaf::download_file_request& msg);
    void on_file_block_request(const leaf::file_block_request& msg);
    void on_block_data_request(const leaf::block_data_request& msg);
    void on_error_response(const leaf::error_response& msg);
    void on_message(const leaf::codec_message& msg);
    void commit_message(const leaf::codec_message& msg);

   private:
    std::string id_;
    leaf::file_context::ptr file_;
    std::shared_ptr<leaf::blake2b> hash_;
    std::shared_ptr<leaf::writer> writer_;
    std::queue<std::vector<uint8_t>> msg_queue_;
};

}    // namespace leaf
#endif
