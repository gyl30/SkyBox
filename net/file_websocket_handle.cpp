#include "log.h"
#include "file_websocket_handle.h"

namespace leaf
{
file_websocket_handle::file_websocket_handle()
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
void file_websocket_handle::on_text_message(const std::string& msg) {}
void file_websocket_handle::on_binary_message(const std::string& bin) {}
void file_websocket_handle::shutdown()
{
    //
    LOG_INFO("shutdown {}", id_);
}

}    // namespace leaf
