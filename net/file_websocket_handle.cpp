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

void file_websocket_handle::startup()
{
    //
    LOG_INFO("startup {}", id_);
}

void file_websocket_handle::on_text_message(const leaf::websocket_session::ptr& session, const std::string& msg)
{
    LOG_INFO("{} on_text_message {}", id_, msg);
    session->write("0hello");
}
void file_websocket_handle::on_binary_message(const leaf::websocket_session::ptr& session, const std::string& bin)
{
    LOG_INFO("{} on_binary_message {}", id_, bin);
    session->write("1hello");
}
void file_websocket_handle::shutdown()
{
    //
    LOG_INFO("shutdown {}", id_);
}

}    // namespace leaf
