#include <utility>
#include <filesystem>

#include <boost/algorithm/hex.hpp>

#include "log/log.h"
#include "file/file.h"
#include "crypt/easy.h"
#include "protocol/codec.h"
#include "file/hash_file.h"
#include "file/download_file_handle.h"

constexpr auto kBlockSize = 128 * 1024;
constexpr auto kHashBlockCount = 10;

namespace leaf
{
download_file_handle::download_file_handle(std::string id) : id_(std::move(id)) { LOG_INFO("create {}", id_); }

download_file_handle::~download_file_handle() { LOG_INFO("destroy {}", id_); }

void download_file_handle::startup() { LOG_INFO("startup {}", id_); }

void download_file_handle::shutdown() { LOG_INFO("shutdown {}", id_); }

void download_file_handle::on_message(const leaf::websocket_session::ptr& session,
                                      const std::shared_ptr<std::vector<uint8_t>>& bytes)
{
    if (bytes == nullptr)
    {
        return;
    }
    if (bytes->empty())
    {
        return;
    }
    auto type = get_message_type(*bytes);
    if (type == leaf::message_type::download_file_request)
    {
        auto req = leaf::deserialize_download_file_request(*bytes);
        on_download_file_request(req);
    }
    if (type == leaf::message_type::keepalive)
    {
        auto k = leaf::deserialize_keepalive_response(*bytes);
        on_keepalive(k);
    }
    if (type == leaf::message_type::login)
    {
        auto msg = leaf::deserialize_login_token(*bytes);
        on_login(msg);
    }

    while (!msg_queue_.empty())
    {
        session->write(msg_queue_.front());
        msg_queue_.pop();
    }
}

void download_file_handle::on_login(const std::optional<leaf::login_token>& message)
{
    if (status_ != wait_login)
    {
        return;
    }
    if (!message.has_value())
    {
        return;
    }
    token_ = message->token;
    LOG_INFO("{} on_login token {}", id_, token_);
    leaf::error_message err;
    err.id = message->id;
    err.error = 0;
    auto bytes = leaf::serialize_error_message(err);
    msg_queue_.push(bytes);
}

void download_file_handle::on_download_file_request(const std::optional<leaf::download_file_request>& message)
{
    if (status_ != wait_download_file_request)
    {
        return;
    }
    if (!message.has_value())
    {
        return;
    }
    const auto& msg = message.value();
    std::string filename = leaf::encode(msg.filename);
    std::string download_file_path = leaf::make_file_path(token_, filename);
    download_file_path = leaf::make_normal_filename(download_file_path);
    LOG_INFO("{} on_download_file_request file {} to {}", id_, msg.filename, download_file_path);
    boost::system::error_code exists_ec;
    bool exist = std::filesystem::exists(download_file_path, exists_ec);
    if (exists_ec)
    {
        LOG_ERROR("{} download_file_request file {} exist error {}", id_, msg.filename, exists_ec.message());
        return;
    }
    if (!exist)
    {
        error_message(boost::system::errc::no_such_file_or_directory, "file not exist");
        LOG_ERROR("{} download_file_request file {} not exist", id_, download_file_path);
        return;
    }

    boost::system::error_code hash_ec;
    std::string h = leaf::hash_file(download_file_path, hash_ec);
    if (hash_ec)
    {
        LOG_ERROR("{} download_file_request file {} hash error {}", id_, msg.filename, hash_ec.message());
        return;
    }
    std::error_code size_ec;
    auto file_size = std::filesystem::file_size(download_file_path, size_ec);
    if (size_ec)
    {
        LOG_ERROR("{} download_file_request file {} size error {}", id_, msg.filename, size_ec.message());
        return;
    }

    assert(!file_ && !reader_ && !hash_);
    reader_ = std::make_shared<leaf::file_reader>(download_file_path);
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
    uint64_t block_count = file_size / kBlockSize;
    uint32_t padding_size = 0;
    if (file_size % kBlockSize != 0)
    {
        padding_size = kBlockSize - (file_size % kBlockSize);
        block_count++;
    }
    hash_ = std::make_shared<leaf::blake2b>();
    file_ = std::make_shared<leaf::file_context>();
    file_->file_path = download_file_path;
    file_->file_size = file_size;
    file_->block_size = kBlockSize;
    file_->block_count = block_count;
    file_->padding_size = padding_size;
    file_->active_block_count = 0;
    file_->content_hash = h;
    file_->filename = msg.filename;
    file_->id = file_id();
    LOG_INFO("{} download_file_request file {} size {} hash {}",
             id_,
             file_->file_path,
             file_->file_size,
             file_->content_hash);
    leaf::download_file_response response;
    response.filename = file_->filename;
    response.id = message->id;
    response.padding_size = padding_size;
    response.block_count = block_count;
    auto bytes = leaf::serialize_download_file_response(response);
    msg_queue_.push(bytes);
    status_ = leaf::download_file_handle::status::file_data;
}

void download_file_handle::update(const leaf::websocket_session::ptr& session)
{
    if (status_ != leaf::download_file_handle::status::file_data)
    {
        return;
    }
    // file data send finish
    if (file_->active_block_count == file_->block_count)
    {
        block_data_finish();
        status_ = leaf::download_file_handle::status::wait_download_file_request;
        return;
    }
    // read block data
    boost::system::error_code ec;
    std::vector<uint8_t> buffer(file_->block_size, '0');
    auto read_size = reader_->read(buffer.data(), buffer.size(), ec);
    if (ec && ec != boost::asio::error::eof)
    {
        // error
        return;
    }
    leaf::file_data fd;
    fd.block_id = file_->active_block_count;
    fd.data.swap(buffer);
    file_->active_block_count++;
    file_->hash_block_count++;
    hash_->update(buffer.data(), read_size);
    // block count hash or eof hash
    if (file_->hash_block_count == kHashBlockCount || ec == boost::asio::error::eof)
    {
        hash_->final();
        fd.hash = hash_->hex();
        file_->hash_block_count = 0;
        hash_ = std::make_shared<leaf::blake2b>();
    }
    // eof reset
    if (ec == boost::asio::error::eof)
    {
        ec = reader_->close();
        // clang-format off
        file_.reset(); reader_.reset(); hash_.reset();
        // clang-format on
        status_ = leaf::download_file_handle::status::wait_download_file_request;
    }
    auto bytes = leaf::serialize_file_data(fd);
    session->write(bytes);
}

void download_file_handle::on_keepalive(const std::optional<leaf::keepalive>& message)
{
    if (!message.has_value())
    {
        return;
    }
    const auto& msg = message.value();
    leaf::keepalive k = msg;
    k.server_timestamp =
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
            .count();
    LOG_DEBUG("{} on_keepalive client {} server_timestamp {} client_timestamp {} token {}",
              id_,
              k.client_id,
              k.server_timestamp,
              k.client_timestamp,
              token_);
}
void download_file_handle::on_error_response(const std::optional<leaf::error_message>& message)
{
    if (!message.has_value())
    {
        return;
    }
    const auto& msg = message.value();
    LOG_INFO("{} on_error_response {}", id_, msg.error);
}
}    // namespace leaf
