#include <utility>
#include <filesystem>
#include "log/log.h"
#include "crypt/easy.h"
#include "crypt/passwd.h"
#include "protocol/codec.h"
#include "protocol/message.h"
#include "file/hash_file.h"
#include "file/upload_file_handle.h"

constexpr auto kBlockSize = 128 * 1024;

namespace leaf
{

upload_file_handle::upload_file_handle(std::string id) : id_(std::move(id))
{
    LOG_INFO("create {}", id_);
    key_ = leaf::passwd_key();
}

upload_file_handle::~upload_file_handle()
{
    //
    LOG_INFO("destroy {}", id_);
}

void upload_file_handle::startup()
{
    //
    LOG_INFO("startup {}", id_);
}

void upload_file_handle::on_message(const leaf::websocket_session::ptr& session,
                                    const std::shared_ptr<std::vector<uint8_t>>& bytes)
{
    auto msg = leaf::deserialize_message(bytes->data(), bytes->size());
    if (!msg)
    {
        return;
    }
    leaf::read_buffer r(bytes->data(), bytes->size());
    uint64_t padding = 0;
    r.read_uint64(&padding);

    uint16_t type = 0;
    r.read_uint16(&type);
    if (type == leaf::to_underlying(leaf::message_type::upload_file_request))
    {
        auto msg = leaf::message::deserialize_upload_file_request(r);
        on_upload_file_request(msg);
    }
    if (type == leaf::to_underlying(leaf::message_type::block_data_response))
    {
        auto msg = leaf::message::deserialize_block_data_response(r);
        on_block_data_response(msg);
    }
    if (type == leaf::to_underlying(leaf::message_type::delete_file_request))
    {
        auto msg = leaf::message::deserialize_delete_file_request(r);
        on_delete_file_request(msg);
    }
    if (type == leaf::to_underlying(leaf::message_type::keepalive))
    {
        auto msg = leaf::message::deserialize_keepalive_response(r);
        on_keepalive(msg);
    }
    if (type == leaf::to_underlying(leaf::message_type::error))
    {
        auto msg = leaf::message::deserialize_error_response(r);
        on_error_response(msg);
    }
    if (type == leaf::to_underlying(leaf::message_type::login_request))
    {
        auto msg = leaf::message::deserialize_login_request(r);
        on_login(msg);
    }

    while (!msg_queue_.empty())
    {
        session->write(msg_queue_.front());
        msg_queue_.pop();
    }
}

void upload_file_handle::commit_message(const leaf::codec_message& msg)
{
    std::vector<uint8_t> bytes = leaf::serialize_message(msg);
    if (bytes.empty())
    {
        return;
    }
    msg_queue_.push(bytes);
}

void upload_file_handle::shutdown()
{
    //
    LOG_INFO("shutdown {}", id_);
}

void upload_file_handle::on_upload_file_request(const std::optional<leaf::upload_file_request>& message)
{
    if (!message.has_value())
    {
        return;
    }
    assert(!file_ && !writer_);
    const auto& msg = message.value();
    std::string filename = leaf::encode(msg.filename);
    std::string upload_file_path = leaf::make_file_path(token_, filename);
    LOG_INFO("{} on_upload_file_request file size {} name {} path {} hash {}",
             id_,
             msg.file_size,
             msg.filename,
             upload_file_path,
             msg.hash);

    std::error_code ec;
    bool exist = std::filesystem::exists(upload_file_path, ec);
    if (ec)
    {
        LOG_ERROR("{} on_upload_file_request file {} exist failed {}", id_, upload_file_path, ec.message());
        return;
    }
    if (exist)
    {
        boost::system::error_code hash_ec;
        std::string hash_hex = leaf::hash_file(upload_file_path, hash_ec);
        if (hash_ec)
        {
            LOG_ERROR("{} on_upload_file_request file {} hash failed {}", id_, upload_file_path, hash_ec.message());
            return;
        }
        if (hash_hex == msg.hash)
        {
            upload_file_exist(msg);
            return;
        }
    }
    assert(file_ == nullptr);
    leaf::upload_file_response response;
    response.block_size = kBlockSize;
    response.filename = msg.filename;
    response.file_id = file_id();
    commit_message(response);
    file_ = std::make_shared<file_context>();
    file_->id = response.file_id;
    file_->file_path = upload_file_path;
    file_->filename = msg.filename;
    file_->file_size = msg.file_size;
    file_->block_size = kBlockSize;
    file_->block_count = msg.file_size / kBlockSize;
    if (msg.file_size % kBlockSize != 0)
    {
        file_->block_count++;
    }
    LOG_DEBUG("{} upload_file_response id {} name {} block count {}",
              id_,
              response.file_id,
              file_->file_path,
              file_->block_count);
    file_->active_block_count = 0;
    hash_ = std::make_shared<leaf::blake2b>();
    writer_ = std::make_shared<leaf::file_writer>(file_->file_path);
    ec = writer_->open();
    if (ec)
    {
        LOG_ERROR("{} open file {} error {}", id_, file_->file_path, ec.message());
        return;
    }
    block_data_request();
}

void upload_file_handle::upload_file_exist(const leaf::upload_file_request& msg)
{
    LOG_INFO("{} on_upload_file_exist file {} hash {}", id_, msg.filename, msg.hash);
    leaf::upload_file_exist exist;
    exist.filename = msg.filename;
    exist.hash = msg.hash;
    commit_message(exist);
}

void upload_file_handle::on_delete_file_request(const std::optional<leaf::delete_file_request>& message)
{
    if (!message.has_value())
    {
        return;
    }
    auto msg = message.value();
    LOG_INFO("{} on_delete_file_request name {}", id_, msg.filename);
}

void upload_file_handle::block_data_request()
{
    leaf::block_data_request request;
    request.file_id = file_->id;
    request.block_id = file_->active_block_count;
    LOG_DEBUG("{} block_data_request id {} block id {}", id_, request.file_id, request.block_id);
    commit_message(request);
}

void upload_file_handle::block_data_finish()
{
    auto ec = writer_->close();
    if (ec)
    {
        LOG_ERROR("{} block_data_finish close file {} error {}", id_, file_->file_path, ec.message());
        return;
    }
    hash_->final();
    LOG_INFO("{} block_data_finish file {} name {} size {} hash {}",
             id_,
             file_->file_path,
             file_->filename,
             writer_->size(),
             hash_->hex());
    block_data_finish1(file_->id, file_->filename, hash_->hex());
    file_ = nullptr;
    writer_.reset();
    hash_.reset();
}
void upload_file_handle::block_data_finish1(uint64_t file_id, const std::string& filename, const std::string& hash)
{
    leaf::block_data_finish finish;
    finish.file_id = file_id;
    finish.filename = filename;
    finish.hash = hash;
    commit_message(finish);
}
void upload_file_handle::on_block_data_response(const std::optional<leaf::block_data_response>& message)
{
    if (!message.has_value())
    {
        return;
    }
    auto msg = message.value();

    assert(file_ && writer_);
    LOG_DEBUG("{} on_block_data_response block id {} file id {} data size {}",
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
    hash_->update(msg.data.data(), msg.data.size());
    LOG_DEBUG("{} on_block_data_response write {} bytes file size {}", id_, write_size, writer_->size());
    if (writer_->size() == file_->file_size)
    {
        block_data_finish();
        return;
    }
    file_->active_block_count = msg.block_id + 1;
    block_data_request();
}
void upload_file_handle::on_login(const std::optional<leaf::login_request>& message)
{
    if (!message.has_value())
    {
        return;
    }
    auto msg = message.value();

    leaf::login_response response;
    response.username = msg.username;
    response.token = msg.token;
    user_ = response.username;
    token_ = response.token;
    LOG_INFO("{} on_login username {} token {}", id_, msg.username, response.token);
    commit_message(response);
}

void upload_file_handle::on_keepalive(const std::optional<leaf::keepalive>& message)
{
    if (!message.has_value())
    {
        return;
    }

    leaf::keepalive k = message.value();
    k.server_timestamp =
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
            .count();
    LOG_DEBUG("{} on_keepalive client {} server_timestamp {} client_timestamp {} token {}",
              id_,
              k.client_id,
              k.server_timestamp,
              k.client_timestamp,
              k.token);
    commit_message(k);
}
void upload_file_handle::on_error_response(const std::optional<leaf::error_response>& message)
{
    if (!message.has_value())
    {
        return;
    }
    auto msg = message.value();

    LOG_INFO("{} on_error_response {}", id_, msg.error);
}

}    // namespace leaf
