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
    LOG_INFO("{} on_create_file_request file size {} name {} hash {}", id_, msg.file_size, msg.filename, msg.hash);
    leaf::create_file_response response;
    response.filename = msg.filename;
    response.file_id = file_id();
    commit_message(response);
    //
    file_ = std::make_shared<file_context>();
    file_->id = response.file_id;
    file_->name = msg.filename;
    file_->file_size = msg.file_size;
    leaf::file_block_request request;
    request.file_id = response.file_id;
    LOG_DEBUG("{} file_block_request id {}", id_, request.file_id);
    commit_message(request);
}

void file_websocket_handle::commit_message(const leaf::codec_message& msg)
{
    std::vector<uint8_t> bytes2;
    if (leaf::serialize_message(msg, &bytes2) != 0)
    {
        return;
    }
    handle_.msg_queue.push(bytes2);
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

void file_websocket_handle::close_file()
{
    assert(writer_);
    auto ec = writer_->close();
    if (ec)
    {
        LOG_ERROR("{} close file {} id {} error {}", id_, file_->name, file_->id, ec.message());
        return;
    }
    writer_ = nullptr;
}
void file_websocket_handle::on_file_block_response(const leaf::file_block_response& msg)
{
    LOG_INFO("{} on_file_block_response id {} size {} count {}", id_, msg.file_id, msg.block_size, msg.block_count);
    if (file_->id != msg.file_id)
    {
        LOG_ERROR("{} on_file_block_response file id {} != {}", id_, msg.file_id, file_->id);
        return;
    }
    if (writer_ != nullptr)
    {
        LOG_WARN("{} change write from {} to {}", id_, writer_->name(), file_->name);
        close_file();
    }
    writer_ = std::make_shared<leaf::file_writer>(file_->name);
    auto ec = writer_->open();
    if (ec)
    {
        LOG_ERROR("{} open file {} error {}", id_, file_->name, ec.message());
        return;
    }
    file_->block_size = msg.block_size;
    file_->block_count = msg.block_count;
    block_data_request();
}

void file_websocket_handle::block_data_request()
{
    leaf::block_data_request request;
    request.file_id = file_->id;
    request.block_id = file_->active_block_count;
    LOG_DEBUG("{} block_data_request id {} block id {}", id_, request.file_id, request.block_id);
    commit_message(request);
}

void file_websocket_handle::block_data_finish()
{
    leaf::block_data_finish finish;
    finish.file_id = file_->id;
    finish.filename = file_->name;
    commit_message(finish);
    LOG_INFO("{} block_data_finish file {} size {}", id_, file_->name, writer_->size());
    auto ec = writer_->close();
    if (ec)
    {
        LOG_ERROR("{} block_data_finish close file {} error {}", id_, file_->name, ec.message());
        return;
    }
}
void file_websocket_handle::on_block_data_response(const leaf::block_data_response& msg)
{
    LOG_INFO("{} on_block_data_response block id {} file id {} data size {}",
             id_,
             msg.block_id,
             msg.file_id,
             msg.data.size());

    if (msg.file_id != file_->id)
    {
        LOG_ERROR("{} on_block_data_response id {} != {}", id_, msg.file_id, file_->id);
        return;
    }
    if (msg.block_id != file_->active_block_count)
    {
        LOG_WARN("{} on_block_data_response id {} != {}", id_, msg.block_id, file_->active_block_count);
        return;
    }
    boost::system::error_code ec;
    auto write_size = writer_->write(msg.data.data(), msg.data.size(), ec);
    if (ec)
    {
        LOG_ERROR("{} on_block_data_response write error {} {}", id_, ec.message(), ec.value());
        return;
    }
    LOG_DEBUG("{} on_block_data_response write {} bytes file size {}", id_, write_size, writer_->size());
    if (writer_->size() == file_->file_size)
    {
        block_data_finish();
        return;
    }
    file_->active_block_count = msg.block_id + 1;
    block_data_request();
}

void file_websocket_handle::on_error_response(const leaf::error_response& msg)
{
    LOG_INFO("{} on_error_response {}", id_, msg.error);
}

}    // namespace leaf
