#include "log.h"
#include "file_websocket_handle.h"

namespace leaf
{
file_websocket_handle::file_websocket_handle(std::string id) : id_(std::move(id))
{
    //
    LOG_INFO("create {}", id_);
}
file_websocket_handle::~file_websocket_handle()
{
    //
    LOG_INFO("destroy {}", id_);
}

void file_websocket_handle::startup() {}
void file_websocket_handle::on_text_message(const std::string& msg) { LOG_INFO("{} on_text_message {}", msg); }
void file_websocket_handle::on_binary_message(const std::string& bin) { LOG_INFO("{} on_binary_message {}", bin); }
void file_websocket_handle::shutdown()
{
    //
    LOG_INFO("shutdown {}", id_);
}

}    // namespace leaf
