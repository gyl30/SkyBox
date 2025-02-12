#include "file/cotrol_file_handle.h"

namespace leaf
{
cotrol_file_handle ::cotrol_file_handle(std::string id) : id_(std::move(id)) {}

cotrol_file_handle::~cotrol_file_handle() {}

void cotrol_file_handle ::startup() {}

void cotrol_file_handle ::shutdown() {}

void cotrol_file_handle ::on_message(const leaf::websocket_session::ptr& session,
                                     const std::shared_ptr<std::vector<uint8_t>>& bytes)
{
}

}    // namespace leaf
