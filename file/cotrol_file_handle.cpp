#include "log/log.h"
#include "protocol/codec.h"
#include "file/cotrol_file_handle.h"

namespace leaf
{
cotrol_file_handle ::cotrol_file_handle(std::string id) : id_(std::move(id)) { LOG_INFO("create {}", id_); }

cotrol_file_handle::~cotrol_file_handle() { LOG_INFO("destroy {}", id_); }

void cotrol_file_handle ::startup() { LOG_INFO("startup {}", id_); }

void cotrol_file_handle ::shutdown() { LOG_INFO("shutdown {}", id_); }

void cotrol_file_handle ::on_message(const leaf::websocket_session::ptr& session,
                                     const std::shared_ptr<std::vector<uint8_t>>& bytes)
{
    auto msg = leaf::deserialize_message(bytes->data(), bytes->size());
    if (!msg)
    {
        LOG_ERROR("{} on_message error {}", id_, bytes->size());
        return;
    }
    LOG_TRACE("{} on_message size {}", id_, bytes->size());
}

}    // namespace leaf
