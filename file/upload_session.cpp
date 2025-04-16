#include <utility>
#include <filesystem>
#include "log/log.h"
#include "file/file.h"
#include "file/hash_file.h"
#include "file/upload_session.h"

constexpr auto kBlockSize = 128 * 1024;
// constexpr auto kHashBlockCount = 10;

namespace leaf
{
upload_session::upload_session(std::string id, leaf::upload_progress_callback cb)
    : id_(std::move(id)), progress_cb_(std::move(cb))
{
}

upload_session::~upload_session() = default;

void upload_session::startup() { LOG_INFO("{} startup", id_); }

void upload_session::shutdown() { LOG_INFO("{} shutdown", id_); }

void upload_session::login(const std::string& token)
{
    LOG_INFO("{} login token {}", id_, token);
    token_.id = seq_++;
    token_.token = token;
    cb_(leaf::serialize_login_token(token_));
}

void upload_session::set_message_cb(std::function<void(std::vector<uint8_t>)> cb) { cb_ = std::move(cb); }

void upload_session::on_message(const std::vector<uint8_t>& bytes)
{
    auto type = leaf::get_message_type(bytes);
    if (type == leaf::message_type::error)
    {
        on_error_message(leaf::deserialize_error_message(bytes));
        return;
    }
}

void upload_session::upload_file_request()
{
    if (file_ == nullptr)
    {
        return;
    }
    boost::system::error_code hash_ec;
    std::string h = leaf::hash_file(file_->file_path, hash_ec);
    if (hash_ec)
    {
        LOG_ERROR("{} upload_file_request file {} hash error {}", id_, file_->file_path, hash_ec.message());
        return;
    }
    std::error_code size_ec;
    auto file_size = std::filesystem::file_size(file_->file_path, size_ec);
    if (size_ec)
    {
        LOG_ERROR("{} upload_file_request file {} size error {}", id_, file_->file_path, size_ec.message());
        return;
    }
    open_file();
    if (reader_ == nullptr)
    {
        return;
    }
    LOG_DEBUG("{} upload_file_request {} size {} hash {}", id_, file_->file_path, file_size, h);
    leaf::upload_file_request u;
    u.id = seq_++;
    u.filename = std::filesystem::path(file_->file_path).filename().string();
    u.block_count = file_size / kBlockSize;
    if (file_size % kBlockSize != 0)
    {
        u.block_count++;
        u.padding_size = kBlockSize - (file_size % kBlockSize);
    }
    cb_(leaf::serialize_upload_file_request(u));
}
void upload_session::open_file()
{
    assert(file_ && !reader_ && !blake2b_);

    reader_ = std::make_shared<leaf::file_reader>(file_->file_path);
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
        LOG_INFO("{} change file from {} to {}", id_, file_->file_path, file->file_path);
    }
    else
    {
        LOG_INFO("{} add file {}", id_, file->file_path);
    }
    padding_files_.push_back(file);
}

void upload_session::update_process_file()
{
    if (file_ != nullptr)
    {
        return;
    }

    if (padding_files_.empty())
    {
        LOG_TRACE("{} padding files empty", id_);
        return;
    }
    file_ = padding_files_.front();
    padding_files_.pop_front();
    LOG_INFO("{} start_file {} size {}", id_, file_->file_path, padding_files_.size());
    upload_file_request();
}
void upload_session::update()
{
    if (!login_)
    {
        return;
    }
    keepalive();
    update_process_file();
    padding_file_event();
}

void upload_session::emit_event(const leaf::upload_event& e)
{
    if (progress_cb_)
    {
        progress_cb_(e);
    }
}

void upload_session::on_error_message(const std::optional<leaf::error_message>& e)
{
    if (!e.has_value())
    {
        return;
    }

    LOG_ERROR("{} error {}", id_, e->error);
}

void upload_session::on_login_token(const std::optional<leaf::login_token>& l)
{
    if (!l.has_value())
    {
        return;
    }
    login_ = true;
    LOG_INFO("{} login_response token {}", id_, l->token);
}
void upload_session::on_keepalive_response(const std::optional<leaf::keepalive>& message)
{
    if (!message.has_value())
    {
        return;
    }
    const auto& msg = message.value();

    auto now =
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
            .count();
    auto diff = now - msg.client_timestamp;

    LOG_DEBUG("{} keepalive_response {} client {} client time {} server time {} diff {} token {}",
              id_,
              msg.id,
              msg.client_id,
              msg.client_timestamp,
              msg.server_timestamp,
              diff,
              token_.token);
}

void upload_session::padding_file_event()
{
    for (const auto& p : padding_files_)
    {
        upload_event e;
        e.file_size = p->file_size;
        e.upload_size = 0;
        e.filename = p->filename;
        emit_event(e);
    }
}

void upload_session::keepalive()
{
    leaf::keepalive k;
    k.id = 0;
    k.client_id = reinterpret_cast<uintptr_t>(this);
    k.client_timestamp =
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
            .count();
    k.server_timestamp = 0;
    cb_(leaf::serialize_keepalive(k));
}

}    // namespace leaf
