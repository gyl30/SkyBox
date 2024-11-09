#include <atomic>
#include <filesystem>
#include <utility>
#include "log.h"
#include "codec.h"
#include "message.h"
#include "hash_file.h"
#include "download_file_handle.h"

namespace leaf
{

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

void download_file_handle::on_download_file_request(const leaf::download_file_request& msg)
{
    LOG_INFO("{} on_download_file_request file {}", id_, msg.filename);
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
