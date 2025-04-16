#include <utility>
#include <filesystem>
#include "log/log.h"
#include "crypt/easy.h"
#include "crypt/passwd.h"
#include "protocol/codec.h"
#include "protocol/message.h"
#include "file/upload_file_handle.h"

constexpr auto kBlockSize = 128 * 1024;

namespace leaf
{

upload_file_handle::upload_file_handle(std::string id, leaf::websocket_session::ptr& session)
    : id_(std::move(id)), session_(session)
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

void upload_file_handle::on_read(boost::beast::error_code ec, const std::vector<uint8_t>& bytes)
{
    if (ec)
    {
        shutdown();
        return;
    }
    if (bytes.empty())
    {
        return;
    }
    auto type = get_message_type(bytes);

    if (type == leaf::message_type::login)
    {
        on_login(leaf::deserialize_login_token(bytes));
    }
    if (type == leaf::message_type::upload_file_request)
    {
        on_upload_file_request(leaf::deserialize_upload_file_request(bytes));
    }
    if (type == leaf::message_type::keepalive)
    {
        on_keepalive(leaf::deserialize_keepalive_response(bytes));
    }
    if (type == leaf::message_type::file_data)
    {
        on_file_data(leaf::deserialize_file_data(bytes));
    }
}

void upload_file_handle::shutdown()
{
    //
    LOG_INFO("shutdown {}", id_);
}

void upload_file_handle::on_upload_file_request(const std::optional<leaf::upload_file_request>& u)
{
    if (state_ != wait_upload_request)
    {
        return;
    }
    if (!u.has_value())
    {
        return;
    }

    assert(!file_ && !writer_);
    std::string filename = leaf::encode(u->filename);
    std::string upload_file_path = leaf::make_file_path(token_, filename);
    upload_file_path = leaf::make_tmp_filename(upload_file_path);
    if (upload_file_path.empty())
    {
        return;
    }
    std::error_code ec;
    bool exist = std::filesystem::exists(upload_file_path, ec);
    if (ec)
    {
        LOG_ERROR("{} on_upload_file_request file {} exist failed {}", id_, upload_file_path, ec.message());
        return;
    }
    if (exist)
    {
        LOG_ERROR("{} on_upload_file_request file {} exist", id_, upload_file_path);
        return;
    }
    auto file_size = u->block_count * kBlockSize;
    if (u->padding_size != 0)
    {
        file_size = kBlockSize - u->padding_size;
    }
    LOG_INFO("{} on_upload_file_request file size {} name {} path {}", id_, file_size, u->filename, upload_file_path);

    assert(file_ == nullptr);
    file_ = std::make_shared<file_context>();
    file_->file_path = upload_file_path;
    file_->filename = u->filename;
    file_->file_size = file_size;
    file_->block_size = kBlockSize;
    file_->block_count = u->block_count;
    file_->active_block_count = 0;
    LOG_DEBUG("{} upload_file_response name {} block count {}", id_, file_->file_path, file_->block_count);
    hash_ = std::make_shared<leaf::blake2b>();
    writer_ = std::make_shared<leaf::file_writer>(file_->file_path);
    ec = writer_->open();
    if (ec)
    {
        LOG_ERROR("{} open file {} error {}", id_, file_->file_path, ec.message());
        return;
    }
    state_ = wait_file_data;
    leaf::error_message e;
    e.error = 0;
    e.id = u->id;
    session_->write(leaf::serialize_error_message(e));
}

void upload_file_handle::on_file_data(const std::optional<leaf::file_data>& d)
{
    if (state_ != wait_file_data)
    {
        return;
    }
    state_ = wait_upload_request;
    if (!d.has_value())
    {
        return;
    }
    hash_->update(d->data.data(), d->data.size());
    boost::system::error_code ec;
    writer_->write(d->data.data(), d->data.size(), ec);
    if (ec)
    {
        LOG_ERROR("{} upload_file write error {}", id_, ec.message());
        return;
    }
    if (!d->hash.empty())
    {
        hash_->final();
        auto hash = hash_->hex();
        if (hash != d->hash)
        {
            LOG_ERROR("{} upload_file hash not match {} {}", id_, hash, d->hash);
            return;
        }
        hash_ = std::make_shared<leaf::blake2b>();
        file_->active_block_count++;
        return;
    }
}
void upload_file_handle::on_login(const std::optional<leaf::login_token>& l)
{
    if (!l.has_value())
    {
        return;
    }
    LOG_INFO("{} on_login token {}", id_, l->token);

    leaf::error_message e;
    e.id = l->id;
    e.error = 0;
    session_->write(leaf::serialize_error_message(e));
}

void upload_file_handle::on_keepalive(const std::optional<leaf::keepalive>& k)
{
    if (!k.has_value())
    {
        return;
    }

    auto sk = k.value();

    sk.server_timestamp =
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
            .count();
    LOG_DEBUG("{} on_keepalive client {} server_timestamp {} client_timestamp {} token {}",
              id_,
              sk.client_id,
              sk.server_timestamp,
              sk.client_timestamp,
              token_);
    session_->write(serialize_keepalive(sk));
}
}    // namespace leaf
