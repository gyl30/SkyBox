#include <utility>
#include <string>
#include <filesystem>
#include "log/log.h"
#include "file/file.h"
#include "file/hash_file.h"
#include "file/download_session.h"

namespace leaf
{
download_session::download_session(std::string id,
                                   leaf::download_progress_callback cb,
                                   leaf::notify_progress_callback notify_cb)
    : id_(std::move(id)), progress_cb_(std::move(cb)), notify_cb_(std::move(notify_cb))
{
}

download_session::~download_session() = default;

void download_session::startup() { LOG_INFO("{} startup", id_); }

void download_session::shutdown() { LOG_INFO("{} shutdown", id_); }

void download_session::set_message_cb(std::function<void(std::vector<uint8_t>)> cb) { cb_ = std::move(cb); }

void download_session::login(const std::string& user, const std::string& pass, const std::string& token)
{
    LOG_INFO("{} login user {} pass {}", id_, user, pass);
    leaf::login_request req;
    req.username = user;
    req.password = pass;
    req.token = token;
    write_message(req);
}

void download_session::on_message(std::vector<uint8_t> msg)
{
    auto c = leaf::deserialize_message(msg.data(), msg.size());
    if (c)
    {
        on_message(c.value());
    }
}
void download_session::on_message(const leaf::codec_message& msg)
{
    struct visitor
    {
        download_session* session_;
        void operator()(const leaf::block_data_finish& msg) { session_->on_block_data_finish(msg); }
        void operator()(const leaf::download_file_response& msg) { session_->on_download_file_response(msg); }
        void operator()(const leaf::file_block_response& msg) { session_->on_file_block_response(msg); }
        void operator()(const leaf::block_data_response& msg) { session_->on_block_data_response(msg); }
        void operator()(const leaf::keepalive& msg) { session_->on_keepalive_response(msg); }
        void operator()(const leaf::error_response& msg) { session_->on_error_response(msg); }
        void operator()(const leaf::login_response& msg) { session_->on_login_response(msg); }
        void operator()(const leaf::upload_file_response& msg) {}
        void operator()(const leaf::delete_file_response& msg) {}
        void operator()(const leaf::upload_file_exist& msg) {}
        void operator()(const leaf::upload_file_request& msg) {}
        void operator()(const leaf::file_block_request& msg) {}
        void operator()(const leaf::block_data_request& msg) {}
        void operator()(const leaf::download_file_request& msg) {}
        void operator()(const leaf::delete_file_request& msg) {}
        void operator()(const leaf::login_request& msg) {}
        void operator()(const leaf::files_request& msg) {}
        void operator()(const leaf::files_response& msg) { session_->on_files_response(msg); }
    };
    std::visit(visitor{this}, msg);
}

void download_session::on_login_response(const leaf::login_response& l)
{
    login_ = true;
    token_ = l.token;
    LOG_INFO("{} login_response user {} token {}", id_, l.username, l.token);
    leaf::notify_event e;
    e.method = "login";
    notify_cb_(e);
}
void download_session::download_file_request()
{
    if (file_ == nullptr)
    {
        return;
    }
    leaf::download_file_request req;
    req.filename = file_->file_path;
    LOG_INFO("{} download_file_request {}", id_, file_->file_path);
    write_message(req);
}

void download_session::on_download_file_response(const leaf::download_file_response& msg)
{
    assert(file_ && !writer_ && !hash_);

    boost::system::error_code exists_ec;
    bool exists = std::filesystem::exists(file_->file_path, exists_ec);
    if (exists_ec)
    {
        LOG_ERROR("{} on_download_file_response file {} exists error {}", id_, file_->file_path, exists_ec.message());
        return;
    }
    if (exists)
    {
        boost::system::error_code hash_ec;
        std::string h = leaf::hash_file(msg.filename, hash_ec);
        if (hash_ec)
        {
            LOG_ERROR("{} on_download_file_response file {} exists retemo {} local hash failed {}",
                      id_,
                      file_->file_path,
                      msg.hash,
                      hash_ec.message());
            return;
        }

        LOG_WARN("{} on_download_file_response file {} exists remote {} local {}", id_, file_->file_path, msg.hash, h);
        return;
    }

    writer_ = std::make_shared<leaf::file_writer>(file_->file_path);
    auto ec = writer_->open();
    if (ec)
    {
        LOG_ERROR("{} on_download_file_response open file {} error {}", id_, file_->file_path, ec.message());
        return;
    }
    hash_ = std::make_shared<leaf::blake2b>();

    assert(file_ && file_->file_path == msg.filename);
    file_->id = msg.file_id;
    file_->file_size = msg.file_size;
    file_->content_hash = msg.hash;
    LOG_INFO("{} download_file_response id {} name {} size {} hash {}",
             id_,
             msg.file_id,
             msg.filename,
             msg.file_size,
             msg.hash);
    leaf::file_block_request req;
    req.file_id = file_->id;
    write_message(req);
    LOG_INFO("{} file_block_request id {}", id_, file_->id);
}

void download_session::on_file_block_response(const leaf::file_block_response& msg)
{
    assert(file_ && file_->id == msg.file_id);
    file_->block_count = msg.block_count;
    file_->block_size = msg.block_size;
    LOG_INFO("{} on_file_block_response id {} count {} size {}", id_, msg.file_id, msg.block_count, msg.block_size);
    block_data_request(file_->active_block_count);
}

void download_session::block_data_request(uint32_t block_id)
{
    leaf::block_data_request req;
    req.block_id = block_id;
    req.file_id = file_->id;
    write_message(req);
    LOG_INFO("{} block_data_request id {} block {}", id_, req.file_id, req.block_id);
}

void download_session::on_block_data_response(const leaf::block_data_response& msg)
{
    assert(file_ && file_->id == msg.file_id);
    assert(file_->active_block_count == msg.block_id);
    assert(file_->active_block_count < file_->block_count);
    boost::system::error_code ec;

    writer_->write(msg.data.data(), msg.data.size(), ec);
    if (ec)
    {
        LOG_ERROR("{} on_block_data_response write error {}", id_, ec.message());
        return;
    }
    hash_->update(msg.data.data(), msg.data.size());
    LOG_INFO("{} on_block_data_response id {} block {} size {} file size {}",
             id_,
             msg.file_id,
             msg.block_id,
             msg.data.size(),
             writer_->size());
    if (msg.block_id == file_->active_block_count)
    {
        file_->active_block_count++;
        block_data_request(file_->active_block_count);
    }
    leaf::download_event e;
    e.id = file_->id;
    e.filename = file_->file_path;
    e.download_size = writer_->size();
    e.file_size = file_->file_size;
    emit_event(e);
}

void download_session::emit_event(const leaf::download_event& e)
{
    if (progress_cb_)
    {
        progress_cb_(e);
    }
}
void download_session::on_block_data_finish(const leaf::block_data_finish& msg)
{
    assert(file_ && file_->id == msg.file_id);
    assert(file_->active_block_count == file_->block_count);
    auto ec = writer_->close();
    if (ec)
    {
        LOG_ERROR("{} on_block_data_finish close file {} error {}", id_, file_->file_path, ec.message());
        return;
    }
    hash_->final();
    LOG_INFO("{} on_block_data_finish file {} size {} hash {}", id_, file_->file_path, writer_->size(), hash_->hex());
    file_.reset();
    writer_.reset();
    hash_.reset();
}
void download_session::write_message(const codec_message& msg)
{
    if (cb_)
    {
        auto bytes = leaf::serialize_message(msg);
        cb_(std::move(bytes));
    }
}
void download_session::add_file(const leaf::file_context::ptr& file)
{
    if (file_ != nullptr)
    {
        LOG_INFO("{} change file from {} to {}", id_, file_->file_path, file->file_path);
    }
    else
    {
        LOG_INFO("{} add file {}", id_, file->file_path);
    }
    padding_files_.push(file);
}

void download_session::update_download_file()
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
    padding_files_.pop();
    LOG_INFO("{} start_file {} size {}", id_, file_->file_path, padding_files_.size());
}

void download_session::update()
{
    if (!login_)
    {
        return;
    }
    files_request();
    keepalive();
    update_download_file();
    download_file_request();
}

void download_session::on_error_response(const leaf::error_response& msg) { LOG_ERROR("{} error {}", id_, msg.error); }

void download_session::on_keepalive_response(const leaf::keepalive& k)
{
    auto now =
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
            .count();
    auto diff = now - k.client_timestamp;

    LOG_DEBUG("{} keepalive_response {} client {} client time {} server time {} diff {} token {}",
              id_,
              k.id,
              k.client_id,
              k.client_timestamp,
              k.server_timestamp,
              diff,
              k.token);
}
void download_session::keepalive()
{
    leaf::keepalive k;
    k.id = 0;
    k.client_id = reinterpret_cast<uintptr_t>(this);
    k.client_timestamp =
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
            .count();
    k.server_timestamp = 0;
    k.token = token_;
    write_message(k);
}
void download_session::files_request()
{
    leaf::files_request r;
    r.token = token_;
    write_message(r);
}
void download_session::on_files_response(const leaf::files_response& res)
{
    LOG_INFO("{} on_files_response {} files {}", id_, res.files.size(), res.token);
    leaf::notify_event e;
    e.method = "files";
    e.data = res;
    notify_cb_(e);
}

}    // namespace leaf
