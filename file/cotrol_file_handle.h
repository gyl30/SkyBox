#ifndef LEAF_FILE_COTROL_FILE_HANDLE_H
#define LEAF_FILE_COTROL_FILE_HANDLE_H

#include <queue>

#include "protocol/codec.h"
#include "protocol/message.h"
#include "net/websocket_handle.h"

namespace leaf
{

class cotrol_file_handle : public websocket_handle
{
   public:
    explicit cotrol_file_handle(std::string id);
    ~cotrol_file_handle() override;

   public:
    void startup() override;
    void shutdown() override;
    std::string type() const override { return "cotrol"; }
    void on_message(const leaf::websocket_session::ptr& session,
                    const std::shared_ptr<std::vector<uint8_t>>& bytes) override;

   private:
    void on_files_request(const std::optional<leaf::files_request>& message);

   private:
    std::string id_;
    std::queue<std::vector<uint8_t>> msg_queue_;
};

}    // namespace leaf

#endif
