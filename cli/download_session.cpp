#include <utility>
#include <filesystem>
#include "log.h"
#include "file.h"
#include "hash_file.h"
#include "download_session.h"

namespace leaf
{
download_session::download_session(std::string id) : id_(std::move(id)) {}

download_session::~download_session() = default;

void download_session::startup() { LOG_INFO("{} startup", id_); }

void download_session::shutdown() { LOG_INFO("{} shutdown", id_); }

void download_session::set_message_cb(std::function<void(const leaf::codec_message&)> cb) { cb_ = std::move(cb); }

void download_session::on_message(const leaf::codec_message& msg)
{
    std::visit(
        [&](auto&& arg)
        {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, leaf::download_file_response>)
            {
                on_download_file_response(arg);
            }
            if constexpr (std::is_same_v<T, leaf::file_block_response>)
            {
                on_file_block_response(arg);
            }
            else if constexpr (std::is_same_v<T, leaf::error_response>)
            {
                error_response(arg);
            }
        },
        msg);
}

void download_session::download_file_request()
{
    if (file_ == nullptr)
    {
        return;
    }
    leaf::download_file_request req;
    req.filename = file_->name;
    write_message(req);
}

void download_session::on_download_file_response(const leaf::download_file_response& msg)
{
    assert(file_ && file_->name == msg.filename);
    file_->id = msg.file_id;
    file_->file_size = msg.file_size;
    file_->src_hash = msg.hash;
    LOG_INFO("{} download_file_response id {} name {} size {} hash {}",
             id_,
             msg.file_id,
             msg.filename,
             msg.file_size,
             msg.hash);
    leaf::file_block_request req;
    req.file_id = file_->id;
    write_message(req);
    LOG_INFO("{} file_block_request id {}", id_, file_->id);
}

void download_session::on_file_block_response(const leaf::file_block_response& msg)
{
    assert(file_ && file_->id == msg.file_id);
    file_->block_count = msg.block_count;
    file_->block_size = msg.block_size;
    LOG_INFO("{} on_file_block_response id {} count {} size {}", id_, msg.file_id, msg.block_count, msg.block_size);
    block_data_request(file_->active_block_count);
}

void download_session::block_data_request(uint32_t block_id)
{
    leaf::block_data_request req;
    req.block_id = block_id;
    req.file_id = file_->id;
    write_message(req);
    LOG_INFO("{} block_data_request id {} block {}", id_, req.file_id, req.block_id);
}
void download_session::write_message(const codec_message& msg)
{
    if (cb_)
    {
        cb_(msg);
    }
}
void download_session::open_file()
{
    assert(file_ && !writer_ && !blake2b_);

    writer_ = std::make_shared<leaf::file_writer>(file_->name);
    auto ec = writer_->open();
    if (ec)
    {
        LOG_ERROR("{} file open error {}", id_, ec.message());
        return;
    }
    blake2b_ = std::make_shared<leaf::blake2b>();
}
void download_session::add_file(const leaf::file_context::ptr& file)
{
    if (file_ != nullptr)
    {
        LOG_INFO("{} change file from {} to {}", id_, file_->name, file->name);
    }
    else
    {
        LOG_INFO("{} add file {}", id_, file->name);
    }
    padding_files_.push(file);
}

void download_session::update()
{
    if (file_ != nullptr)
    {
        return;
    }

    if (padding_files_.empty())
    {
        LOG_INFO("{} padding files empty", id_);
        return;
    }
    file_ = padding_files_.front();
    padding_files_.pop();
    LOG_INFO("{} start_file {} size {}", id_, file_->name, padding_files_.size());
    download_file_request();
}

void download_session::error_response(const leaf::error_response& msg) { LOG_ERROR("{} error {}", id_, msg.error); }
}    // namespace leaf
