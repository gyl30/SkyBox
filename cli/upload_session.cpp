#include <utility>
#include <filesystem>
#include "log.h"
#include "file.h"
#include "hash_file.h"
#include "upload_session.h"

namespace leaf
{
upload_session::upload_session(std::string id) : id_(std::move(id)) {}

upload_session::~upload_session() {}

void upload_session::startup() {}
void upload_session::shutdown() {}

void upload_session::set_message_cb(std::function<void(const leaf::codec_message&)> cb) { cb_ = std::move(cb); }

void upload_session::on_message(const leaf::codec_message& msg)
{
    std::visit(
        [&](auto&& arg)
        {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, leaf::file_block_request>)
            {
                file_block_request(arg);
            }
            else if constexpr (std::is_same_v<T, leaf::block_data_request>)
            {
                block_data_request(arg);
            }
            else if constexpr (std::is_same_v<T, leaf::create_file_response>)
            {
                create_file_response(arg);
            }
            else if constexpr (std::is_same_v<T, leaf::delete_file_response>)
            {
                delete_file_response(arg);
            }
            else if constexpr (std::is_same_v<T, leaf::block_data_finish>)
            {
                block_data_finish(arg);
            }
            else if constexpr (std::is_same_v<T, leaf::create_file_exist>)
            {
                create_file_exist(arg);
            }
            else if constexpr (std::is_same_v<T, leaf::error_response>)
            {
                error_response(arg);
            }
            else
            {
            }
        },
        msg);
}
void upload_session::create_file_response(const leaf::create_file_response& msg)
{
    file_->id = msg.file_id;
    LOG_INFO("{} create_file_response id {} name {}", id_, msg.file_id, msg.filename);
}
void upload_session::delete_file_response(const leaf::delete_file_response& msg)
{
    LOG_INFO("{} delete_file_response name {}", id_, msg.filename);
}

void upload_session::create_file_request()
{
    if (file_ == nullptr)
    {
        return;
    }
    boost::system::error_code hash_ec;
    std::string h = leaf::hash_file(file_->name, hash_ec);
    if (hash_ec)
    {
        LOG_ERROR("{} create_file_request file {} hash error {}", id_, file_->name, hash_ec.message());
        return;
    }
    std::error_code size_ec;
    auto file_size = std::filesystem::file_size(file_->name, size_ec);
    if (size_ec)
    {
        LOG_ERROR("{} create_file_request file {} size error {}", id_, file_->name, size_ec.message());
        return;
    }
    open_file();
    if (reader_ == nullptr)
    {
        return;
    }
    LOG_DEBUG("{} create_file_request {} size {} hash {}", id_, file_->name, file_size, h);
    leaf::create_file_request create;
    create.file_size = file_size;
    create.hash = h;
    create.op = leaf::to_underlying(op_type::upload);
    create.filename = std::filesystem::path(file_->name).filename().string();
    file_->file_size = file_size;

    write_message(create);
}
void upload_session::write_message(const codec_message& msg)
{
    if (cb_)
    {
        cb_(msg);
    }
}
void upload_session::open_file()
{
    assert(file_ && !reader_ && !blake2b_);

    reader_ = std::make_shared<leaf::file_reader>(file_->name);
    auto ec = reader_->open();
    if (ec)
    {
        LOG_ERROR("{} file open error {}", id_, ec.message());
        return;
    }
    blake2b_ = std::make_shared<leaf::blake2b>();
}
void upload_session::add_file(const leaf::file_context::ptr& file)
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

void upload_session::update()
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
    create_file_request();
}

void upload_session::block_data_request(const leaf::block_data_request& msg)
{
    assert(file_ && reader_ && blake2b_);
    LOG_DEBUG("{} block_data_request id {} block {}", id_, msg.file_id, msg.block_id);
    if (msg.block_id != file_->active_block_count)
    {
        LOG_ERROR(
            "{} block_data_request block {} less than send block {}", id_, msg.block_id, file_->active_block_count);
        return;
    }

    boost::system::error_code ec;
    std::vector<uint8_t> buffer(file_->block_size, '0');
    auto read_size = reader_->read(buffer.data(), buffer.size(), ec);
    if (ec)
    {
        LOG_ERROR("{} block_data_request {} error {}", id_, msg.block_id, ec.message());
        return;
    }
    blake2b_->update(buffer.data(), read_size);
    buffer.resize(read_size);
    file_->active_block_count = msg.block_id;
    LOG_DEBUG("{} block_data_request id {} block {} size {}", id_, msg.file_id, msg.block_id, read_size);
    leaf::block_data_response response;
    response.file_id = msg.file_id;
    response.block_id = msg.block_id;
    response.data = std::move(buffer);
    write_message(response);
    file_->active_block_count = msg.block_id + 1;
}
void upload_session::file_block_request(const leaf::file_block_request& msg)
{
    if (msg.file_id != file_->id)
    {
        LOG_ERROR("{} file id not match {} {}", id_, msg.file_id, file_->id);
        return;
    }
    constexpr auto kBlockSize = 128 * 1024;
    uint64_t block_count = file_->file_size / kBlockSize;
    if (file_->file_size % kBlockSize != 0)
    {
        block_count++;
    }
    leaf::file_block_response response;
    response.block_count = block_count;
    response.block_size = kBlockSize;
    response.file_id = file_->id;
    file_->block_size = kBlockSize;
    LOG_INFO("{} file_block_request id {} count {} size {}", id_, file_->id, block_count, kBlockSize);
    write_message(response);
}

void upload_session::block_data_finish(const leaf::block_data_finish& msg)
{
    assert(file_ && reader_ && blake2b_);
    blake2b_->final();
    LOG_INFO("{} block_data_finish id {} file {} hash {} size {} read size {} read hash {}",
             id_,
             msg.file_id,
             msg.filename,
             msg.hash,
             file_->file_size,
             reader_->size(),
             blake2b_->hex());

    file_ = nullptr;
    blake2b_.reset();
    auto r = reader_;
    reader_ = nullptr;
    auto ec = r->close();
    if (ec)
    {
        LOG_ERROR("{} close file error {}", id_, ec.message());
        return;
    }
}
void upload_session::create_file_exist(const leaf::create_file_exist& exist)
{
    assert(file_ && reader_ && blake2b_);
    auto ec = reader_->close();
    if (ec)
    {
        LOG_ERROR("{} close file error {}", id_, ec.message());
        return;
    }
    reader_.reset();
    blake2b_->final();
    blake2b_.reset();

    std::string filename = std::filesystem::path(file_->name).filename().string();
    assert(filename == exist.filename);
    file_ = nullptr;
    LOG_INFO("{} create_file_exist {} hash {}", id_, exist.filename, exist.hash);
}
void upload_session::error_response(const leaf::error_response& msg) { LOG_ERROR("{} error {}", id_, msg.error); }
}    // namespace leaf
