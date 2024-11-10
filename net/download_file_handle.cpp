#include <atomic>
#include <filesystem>
#include <utility>
#include "log.h"
#include "codec.h"
#include "file.h"
#include "message.h"
#include "hash_file.h"
#include "download_file_handle.h"

namespace leaf
{
static uint64_t file_id()
{
    static std::atomic<uint64_t> id = 0xffff1;
    return ++id;
}

download_file_handle::download_file_handle(std::string id) : id_(std::move(id)) {}

download_file_handle::~download_file_handle() = default;

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
            if constexpr (std::is_same_v<T, leaf::error_response>)
            {
                on_error_response(arg);
            }
        },
        msg);
}

void download_file_handle::open_file()
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

void download_file_handle::on_download_file_request(const leaf::download_file_request& msg)
{
    LOG_INFO("{} on_download_file_request file {}", id_, msg.filename);
    file_ = std::make_shared<leaf::file_context>();
    file_->name = msg.filename;
    boost::system::error_code hash_ec;
    std::string h = leaf::hash_file(file_->name, hash_ec);
    if (hash_ec)
    {
        LOG_ERROR("{} download_file_request file {} hash error {}", id_, file_->name, hash_ec.message());
        return;
    }
    std::error_code size_ec;
    auto file_size = std::filesystem::file_size(file_->name, size_ec);
    if (size_ec)
    {
        LOG_ERROR("{} download_file_request file {} size error {}", id_, file_->name, size_ec.message());
        return;
    }
    open_file();
    if (writer_ == nullptr)
    {
        return;
    }
    file_->file_size = file_size;
    file_->src_hash = h;
    LOG_INFO("{} download_file_request file {} size {} hash {}", id_, file_->name, file_->file_size, file_->src_hash);
    leaf::download_file_response response;
    response.filename = file_->name;
    response.file_id = file_id();
    response.file_size = file_->file_size;
    response.hash = file_->src_hash;
    commit_message(response);
}

void download_file_handle::on_file_block_request(const leaf::file_block_request& msg)
{
    LOG_INFO("{} on_file_block_request file {}", id_, msg.file_id);
}

void download_file_handle::on_block_data_request(const leaf::block_data_request& msg)
{
    LOG_INFO("{} on_block_data_request file {} block id ", id_, msg.file_id, msg.block_id);
}

void download_file_handle::on_error_response(const leaf::error_response& msg)
{
    LOG_INFO("{} on_error_response {}", id_, msg.error);
}

}    // namespace leaf
