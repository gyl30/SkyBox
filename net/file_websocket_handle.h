#include "codec.h"
#include "message.h"
#include "websocket_handle.h"

namespace leaf
{

class file_websocket_handle : public websocket_handle
{
   public:
    explicit file_websocket_handle(std::string id);
    ~file_websocket_handle() override;

   public:
    void startup() override;
    void on_text_message(const leaf::websocket_session::ptr& session,
                         const std::shared_ptr<std::vector<uint8_t>>& msg) override;
    void on_binary_message(const leaf::websocket_session::ptr& session,
                           const std::shared_ptr<std::vector<uint8_t>>& msg) override;
    void shutdown() override;

   private:
    void on_create_file_request(const leaf::create_file_request& msg);
    void on_delete_file_request(const leaf::delete_file_request& msg);
    void on_file_block_request(const leaf::file_block_request& msg);
    void on_block_data_request(const leaf::block_data_request& msg);
    void on_create_file_response(const leaf::create_file_response& msg);
    void on_delete_file_response(const leaf::delete_file_response& msg);
    void on_file_block_response(const leaf::file_block_response& msg);
    void on_block_data_response(const leaf::block_data_response& msg);
    void on_error_response(const leaf::error_response& msg);

   private:
    std::string id_;
    std::vector<uint8_t> bytes_;
    leaf::codec_handle handle_;
};

}    // namespace leaf
