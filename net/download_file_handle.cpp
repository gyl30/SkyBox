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

void download_file_handle::startup() {}

void download_file_handle::shutdown() {}

void download_file_handle::on_message(const leaf::websocket_session::ptr& session,
                                      const std::shared_ptr<std::vector<uint8_t>>& bytes)
{
}

void download_file_handle::on_download_file_request(const leaf::download_file_request& msg) {}

void download_file_handle::on_file_block_request(const leaf::file_block_request& msg) {}

void download_file_handle::on_block_data_request(const leaf::block_data_request& msg) {}

void download_file_handle::block_data_request() {}

void download_file_handle::on_error_response(const leaf::error_response& msg) {}

void download_file_handle::on_message(const leaf::codec_message& msg) {}

void download_file_handle::commit_message(const leaf::codec_message& msg) {}

}    // namespace leaf
