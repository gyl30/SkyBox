#include <atomic>
#include "log.h"
#include "codec.h"
#include "message.h"
#include "file_websocket_handle.h"

namespace leaf
{
static uint64_t file_id()
{
    static std::atomic<uint64_t> id = 0xff1;
    return ++id;
}
file_websocket_handle::file_websocket_handle(std::string id) : id_(std::move(id))
{
    // clang-format off
    handle_.create_file_request = std::bind(&file_websocket_handle::on_create_file_request, this, std::placeholders::_1);
    handle_.delete_file_request = std::bind(&file_websocket_handle::on_delete_file_request, this, std::placeholders::_1);
    handle_.file_block_request = std::bind(&file_websocket_handle::on_file_block_request, this, std::placeholders::_1);
    handle_.block_data_request = std::bind(&file_websocket_handle::on_block_data_request, this, std::placeholders::_1);
    handle_.create_file_response = std::bind(&file_websocket_handle::on_create_file_response, this, std::placeholders::_1);
    handle_.delete_file_response = std::bind(&file_websocket_handle::on_delete_file_response, this, std::placeholders::_1);
    handle_.file_block_response = std::bind(&file_websocket_handle::on_file_block_response, this, std::placeholders::_1);
    handle_.block_data_response = std::bind(&file_websocket_handle::on_block_data_response, this, std::placeholders::_1);
    handle_.error_response = std::bind(&file_websocket_handle::on_error_response, this, std::placeholders::_1);
    // clang-format on
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
    file_ = std::make_shared<file_item>();
    LOG_INFO("startup {}", id_);
}

void file_websocket_handle::on_text_message(const leaf::websocket_session::ptr& /*session*/,
                                            const std::shared_ptr<std::vector<uint8_t>>& msg)
{
    LOG_INFO("{} on_text_message {}", id_, msg->size());
}
void file_websocket_handle::on_binary_message(const leaf::websocket_session::ptr& session,
                                              const std::shared_ptr<std::vector<uint8_t>>& msg)
{
    LOG_INFO("{} on_binary_message {}", id_, msg->size());
    if (leaf::deserialize_message(msg->data(), msg->size(), &handle_) != 0)
    {
        return;
    }
    while (!handle_.msg_queue.empty())
    {
        session->write(handle_.msg_queue.front());
        handle_.msg_queue.pop();
    }
}
void file_websocket_handle::shutdown()
{
    //
    LOG_INFO("shutdown {}", id_);
}

void file_websocket_handle::on_create_file_request(const leaf::create_file_request& msg)
{
    LOG_INFO("{} on_create_file_request file size {} name {}", id_, msg.file_size, msg.filename);
    leaf::create_file_response response;
    response.filename = msg.filename;
    response.file_id = file_id();
    {
        std::vector<uint8_t> bytes;
        if (leaf::serialize_message(response, &bytes) != 0)
        {
            return;
        }
        handle_.msg_queue.push(bytes);
    }
    file_->id = response.file_id;
    file_->name = msg.filename;
    leaf::file_block_request request;
    request.file_id = response.file_id;
    {
        std::vector<uint8_t> bytes2;
        if (leaf::serialize_message(request, &bytes2) != 0)
        {
            return;
        }
        handle_.msg_queue.push(bytes2);
    }
}

void file_websocket_handle::on_delete_file_request(const leaf::delete_file_request& msg)
{
    LOG_INFO("{} on_delete_file_request name {}", id_, msg.filename);
}
void file_websocket_handle::on_file_block_request(const leaf::file_block_request& msg)
{
    LOG_INFO("{} on_file_block_request file id {}", id_, msg.file_id);
}
void file_websocket_handle::on_block_data_request(const leaf::block_data_request& msg)
{
    LOG_INFO("{} on_block_data_request block id {} file id {}", id_, msg.block_id, msg.file_id);
}
void file_websocket_handle::on_create_file_response(const leaf::create_file_response& msg)
{
    LOG_INFO("{} on_create_file_response file id {} name {}", id_, msg.file_id, msg.filename);
}
void file_websocket_handle::on_delete_file_response(const leaf::delete_file_response& msg)
{
    LOG_INFO("{} on_delete_file_response name {}", id_, msg.filename);
}
void file_websocket_handle::on_file_block_response(const leaf::file_block_response& msg)
{
    if (file_->id != msg.file_id)
    {
        LOG_ERROR("{} on_file_block_response file id {} != {}", id_, msg.file_id, file_->id);
        return;
    }
    file_->block_size = msg.block_size;
    file_->block_count = msg.block_count;
    LOG_INFO("{} on_file_block_response block size {} block count {}", id_, msg.block_size, msg.block_count);
    //
}
void file_websocket_handle::on_block_data_response(const leaf::block_data_response& msg)
{
    LOG_INFO("{} on_block_data_response block id {} file id {}", id_, msg.block_id, msg.file_id);
}
void file_websocket_handle::on_error_response(const leaf::error_response& msg)
{
    LOG_INFO("{} on_error_response {}", id_, msg.error);
}

}    // namespace leaf
