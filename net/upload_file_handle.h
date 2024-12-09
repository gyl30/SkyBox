#ifndef LEAF_UPLOAD_FILE_HANDLE_H
#define LEAF_UPLOAD_FILE_HANDLE_H

#include <queue>
#include "file.h"
#include "codec.h"
#include "message.h"
#include "blake2b.h"
#include "file_context.h"
#include "websocket_handle.h"

namespace leaf
{

class upload_file_handle : public websocket_handle
{
   public:
    explicit upload_file_handle(std::string id);
    ~upload_file_handle() override;

   public:
    void startup() override;
    void on_message(const leaf::websocket_session::ptr& session,
                    const std::shared_ptr<std::vector<uint8_t>>& bytes) override;
    void shutdown() override;

   private:
    void on_keepalive(const leaf::keepalive& msg);
    void on_upload_file_request(const leaf::upload_file_request& msg);
    void on_delete_file_request(const leaf::delete_file_request& msg);
    void on_file_block_response(const leaf::file_block_response& msg);
    void on_block_data_response(const leaf::block_data_response& msg);
    void on_error_response(const leaf::error_response& msg);
    void block_data_request();
    void block_data_finish();
    void block_data_finish1(uint64_t file_id, const std::string& filename, const std::string& hash);
    void upload_file_exist(const leaf::upload_file_request& msg);
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
