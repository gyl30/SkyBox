#include <utility>
#include <filesystem>
#include "log/log.h"
#include "crypt/easy.h"
#include "crypt/passwd.h"
#include "config/config.h"
#include "protocol/codec.h"
#include "protocol/message.h"
#include "file/upload_file_handle.h"

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
    // clang-format off
    session_->set_read_cb([this, self = shared_from_this()](boost::beast::error_code ec, const std::vector<uint8_t>& bytes) { on_read(ec, bytes); });
    session_->set_write_cb([this, self = shared_from_this()](boost::beast::error_code ec, std::size_t bytes_transferred) { on_write(ec, bytes_transferred); });
    session_->startup();
    // clang-format on
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
    if (type == leaf::message_type::ack)
    {
        on_ack(leaf::deserialize_ack(bytes));
    }
    if (type == leaf::message_type::done)
    {
        on_file_done(leaf::deserialize_done(bytes));
    }
    if (type == leaf::message_type::file_data)
    {
        on_file_data(leaf::deserialize_file_data(bytes));
    }
}

void upload_file_handle::on_write(boost::beast::error_code ec, std::size_t /*transferred*/)
{
    if (ec)
    {
        shutdown();
        return;
    }
}
void upload_file_handle ::shutdown()
{
    std::call_once(shutdown_flag_, [this, self = shared_from_this()]() { safe_shutdown(); });
}

void upload_file_handle::safe_shutdown()
{
    session_->shutdown();
    session_.reset();
    LOG_INFO("shutdown {}", id_);
}

void upload_file_handle::on_upload_file_request(const std::optional<leaf::upload_file_request>& req)
{
    if (state_ != wait_upload_request)
    {
        LOG_ERROR("{} upload_file request state failed", id_);
        return;
    }
    if (!req.has_value())
    {
        return;
    }

    assert(!file_ && !writer_);
    std::string filename = leaf::encode(req->filename);
    std::string upload_file_path = leaf::make_file_path(token_, filename);
    upload_file_path = leaf::make_normal_filename(upload_file_path);
    std::error_code ec;
    bool exist = std::filesystem::exists(upload_file_path, ec);
    if (ec)
    {
        LOG_ERROR("{} upload_file request file {} exist failed {}", id_, upload_file_path, ec.message());
        reset_state();
        error_message(req->id, ec.value());
        return;
    }
    if (exist)
    {
        LOG_ERROR("{} upload_file request file {} exist", id_, upload_file_path);
        reset_state();
        ec = boost::system::errc::make_error_code(boost::system::errc::file_exists);
        error_message(req->id, ec.value());
        return;
    }
    LOG_INFO(
        "{} upload_file request file size {} name {} path {}", id_, req->filesize, req->filename, upload_file_path);

    assert(file_ == nullptr);
    file_ = std::make_shared<file_context>();
    file_->file_path = upload_file_path;
    file_->filename = req->filename;
    file_->file_size = req->filesize;
    file_->hash_count = 0;
    hash_ = std::make_shared<leaf::blake2b>();
    writer_ = std::make_shared<leaf::file_writer>(file_->file_path);
    ec = writer_->open();
    if (ec)
    {
        LOG_ERROR("{} upload_file open file {} error {}", id_, file_->file_path, ec.message());
        reset_state();
        error_message(req->id, ec.value());
        return;
    }
    state_ = wait_ack;
    leaf::upload_file_response ufr;
    ufr.id = req->id;
    ufr.filename = req->filename;
    session_->write(leaf::serialize_upload_file_response(ufr));
}

void upload_file_handle::on_ack(const std::optional<leaf::ack>& /*a*/)
{
    LOG_INFO("{} upload_file recv ack", id_);
    assert(state_ == wait_ack);
    state_ = wait_file_data;
}
void upload_file_handle::on_file_data(const std::optional<leaf::file_data>& d)
{
    if (state_ != wait_file_data)
    {
        LOG_ERROR("{} upload_file state failed", id_);
        reset_state();
        shutdown();
        return;
    }
    if (!d.has_value())
    {
        reset_state();
        shutdown();
        return;
    }
    assert(d->data.size() <= kBlockSize);
    boost::system::error_code ec;
    writer_->write_at(writer_->size(), d->data.data(), d->data.size(), ec);
    if (ec)
    {
        LOG_ERROR("{} upload_file write error {}", id_, ec.message());
        reset_state();
        return;
    }
    file_->hash_count++;
    hash_->update(d->data.data(), d->data.size());

    LOG_DEBUG("{} upload_file {} hash count {} hash {} data size {} write size {}",
              id_,
              file_->filename,
              file_->hash_count,
              d->hash.empty() ? "empty" : d->hash,
              d->data.size(),
              writer_->size());

    if (!d->hash.empty())
    {
        hash_->final();
        auto hash = hash_->hex();
        if (hash != d->hash)
        {
            LOG_ERROR("{} upload_file hash not match {} {}", id_, hash, d->hash);
            reset_state();
            return;
        }
        file_->hash_count = 0;
        hash_ = std::make_shared<leaf::blake2b>();
    }
    if (file_->file_size == writer_->size())
    {
        LOG_INFO("{} upload_file {} done", id_, file_->file_path);
        reset_state();
    }
}

void upload_file_handle::on_file_done(const std::optional<leaf::done>& /*d*/)
{
    LOG_INFO("{} upload_file done", id_);
    assert(state_ == wait_upload_request);
}

void upload_file_handle::error_message(uint32_t id, int32_t error_code)
{
    leaf::error_message e;
    e.id = id;
    e.error = error_code;
    session_->write(leaf::serialize_error_message(e));
}

void upload_file_handle::reset_state()
{
    if (file_)
    {
        file_.reset();
    }
    if (writer_)
    {
        auto ec = writer_->close();
        boost::ignore_unused(ec);
        writer_.reset();
    }
    if (hash_)
    {
        hash_.reset();
    }
    state_ = wait_upload_request;
}
void upload_file_handle::on_login(const std::optional<leaf::login_token>& l)
{
    if (!l.has_value())
    {
        return;
    }
    LOG_INFO("{} on_login token {}", id_, l->token);
    token_ = l->token;
    state_ = wait_upload_request;
    session_->write(leaf::serialize_login_token(l.value()));
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
