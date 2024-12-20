#include <atomic>
#include <utility>
#include <filesystem>
#include "log/log.h"
#include "protocol/codec.h"
#include "file/file.h"
#include "protocol/message.h"
#include "file/hash_file.h"
#include "file/download_file_handle.h"

namespace leaf
{
static uint64_t file_id()
{
    static std::atomic<uint64_t> id = 0xffff1;
    return ++id;
}

download_file_handle::download_file_handle(std::string id) : id_(std::move(id)) { LOG_INFO("create {}", id_); }

download_file_handle::~download_file_handle() { LOG_INFO("destroy {}", id_); }

void download_file_handle::startup() { LOG_INFO("startup {}", id_); }

void download_file_handle::shutdown() { LOG_INFO("shutdown {}", id_); }

void download_file_handle::on_message(const leaf::websocket_session::ptr& session,
                                      const std::shared_ptr<std::vector<uint8_t>>& bytes)
{
    auto msg = leaf::deserialize_message(bytes->data(), bytes->size());
    if (!msg)
    {
        return;
    }
    on_message(msg.value());
    while (!msg_queue_.empty())
    {
        session->write(msg_queue_.front());
        msg_queue_.pop();
    }
}

void download_file_handle::commit_message(const leaf::codec_message& msg)
{
    std::vector<uint8_t> bytes = leaf::serialize_message(msg);
    if (bytes.empty())
    {
        return;
    }
    msg_queue_.push(bytes);
}

void download_file_handle::on_message(const leaf::codec_message& msg)
{
    std::visit(
        [this](auto&& arg)
        {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, leaf::download_file_request>)
            {
                on_download_file_request(arg);
            }
            if constexpr (std::is_same_v<T, leaf::file_block_request>)
            {
                on_file_block_request(arg);
            }
            if constexpr (std::is_same_v<T, leaf::block_data_request>)
            {
                on_block_data_request(arg);
            }
            if constexpr (std::is_same_v<T, leaf::keepalive>)
            {
                on_keepalive(arg);
            }
            if constexpr (std::is_same_v<T, leaf::error_response>)
            {
                on_error_response(arg);
            }
        },
        msg);
}

void download_file_handle::on_download_file_request(const leaf::download_file_request& msg)
{
    LOG_INFO("{} on_download_file_request file {}", id_, msg.filename);

    boost::system::error_code exists_ec;
    bool exist = std::filesystem::exists(msg.filename, exists_ec);
    if (exists_ec)
    {
        LOG_ERROR("{} download_file_request file {} exist error {}", id_, msg.filename, exists_ec.message());
        return;
    }
    if (!exist)
    {
        LOG_ERROR("{} download_file_request file {} not exist", id_, msg.filename);
        return;
    }

    boost::system::error_code hash_ec;
    std::string h = leaf::hash_file(msg.filename, hash_ec);
    if (hash_ec)
    {
        LOG_ERROR("{} download_file_request file {} hash error {}", id_, msg.filename, hash_ec.message());
        return;
    }
    std::error_code size_ec;
    auto file_size = std::filesystem::file_size(msg.filename, size_ec);
    if (size_ec)
    {
        LOG_ERROR("{} download_file_request file {} size error {}", id_, msg.filename, size_ec.message());
        return;
    }

    assert(!file_ && !reader_ && !hash_);
    reader_ = std::make_shared<leaf::file_reader>(msg.filename);
    auto ec = reader_->open();
    if (ec)
    {
        LOG_ERROR("{} file open error {}", id_, ec.message());
        return;
    }
    if (reader_ == nullptr)
    {
        return;
    }
    hash_ = std::make_shared<leaf::blake2b>();
    constexpr auto kBlockSize = 128 * 1024;
    uint64_t block_count = file_size / kBlockSize;
    if (file_size % kBlockSize != 0)
    {
        block_count++;
    }
    file_ = std::make_shared<leaf::file_context>();
    file_->file_path = msg.filename;
    file_->file_size = file_size;
    file_->block_size = kBlockSize;
    file_->block_count = block_count;
    file_->active_block_count = 0;
    file_->content_hash = h;
    file_->id = file_id();
    LOG_INFO(
        "{} download_file_request file {} size {} hash {}", id_, file_->file_path, file_->file_size, file_->content_hash);
    leaf::download_file_response response;
    response.filename = file_->file_path;
    response.file_id = file_->id;
    response.file_size = file_->file_size;
    response.hash = file_->content_hash;
    commit_message(response);
}

void download_file_handle::on_file_block_request(const leaf::file_block_request& msg)
{
    assert(file_ && msg.file_id == file_->id);
    leaf::file_block_response response;
    response.block_count = file_->block_count;
    response.block_size = file_->block_size;
    response.file_id = file_->id;
    commit_message(response);
    LOG_INFO(
        "{} on_file_block_request id {} count {} size {}", id_, msg.file_id, response.block_count, response.block_size);
}

void download_file_handle::on_block_data_request(const leaf::block_data_request& msg)
{
    assert(file_ && reader_ && hash_);
    LOG_DEBUG("{} block_data_request id {} block {}", id_, msg.file_id, msg.block_id);
    if (msg.block_id != file_->active_block_count)
    {
        LOG_ERROR(
            "{} block_data_request block {} less than send block {}", id_, msg.block_id, file_->active_block_count);
        return;
    }
    if (msg.block_id == file_->block_count)
    {
        LOG_INFO("{} block_data_request id {} block id {} block count {} finish",
                 id_,
                 msg.file_id,
                 msg.block_id,
                 file_->block_count);
        block_data_finish();
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
    hash_->update(buffer.data(), read_size);
    buffer.resize(read_size);
    leaf::block_data_response response;
    response.file_id = msg.file_id;
    response.block_id = msg.block_id;
    response.data = std::move(buffer);
    commit_message(response);
    file_->active_block_count += 1;
    LOG_DEBUG("{} block_data_request id {} block {} read size {}", id_, msg.file_id, msg.block_id, read_size);
}

void download_file_handle::block_data_finish()
{
    auto ec = reader_->close();
    if (ec)
    {
        LOG_ERROR("{} block_data_finish close file {} error {}", id_, file_->file_path, ec.message());
        return;
    }
    hash_->final();
    LOG_INFO("{} block_data_finish file {} size {} hash {}", id_, file_->file_path, reader_->size(), hash_->hex());
    block_data_finish1(file_->id, file_->file_path, hash_->hex());
    file_ = nullptr;
    reader_.reset();
    hash_.reset();
}
void download_file_handle::block_data_finish1(uint64_t file_id, const std::string& filename, const std::string& hash)
{
    leaf::block_data_finish finish;
    finish.file_id = file_id;
    finish.filename = filename;
    finish.hash = hash;
    commit_message(finish);
}

void download_file_handle::on_keepalive(const leaf::keepalive& msg)
{
    leaf::keepalive k = msg;
    k.server_timestamp =
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
            .count();
    commit_message(k);
}
void download_file_handle::on_error_response(const leaf::error_response& msg)
{
    LOG_INFO("{} on_error_response {}", id_, msg.error);
}

}    // namespace leaf
