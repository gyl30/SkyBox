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

void download_session::login(const std::string& user, const std::string& pass, const leaf::login_token& l)
{
    LOG_INFO("{} login user {} pass {}", id_, user, pass);
    leaf::login_request req;
    req.username = user;
    req.password = pass;
    req.token = l.token;
    token_ = l;
    write_message(req);
}

void download_session::on_message(const std::vector<uint8_t>& bytes)
{
    auto msg = leaf::deserialize_message(bytes.data(), bytes.size());
    if (!msg)
    {
        return;
    }
    leaf::read_buffer r(bytes.data(), bytes.size());
    uint64_t padding = 0;
    r.read_uint64(&padding);
    uint16_t type = 0;
    r.read_uint16(&type);
    if (type == leaf::to_underlying(leaf::message_type::download_file_response))
    {
        on_download_file_response(leaf::message::deserialize_download_file_response(r));
    }
    if (type == leaf::to_underlying(leaf::message_type::block_data_finish))
    {
        on_block_data_finish(leaf::message::deserialize_block_data_finish(r));
    }
    if (type == leaf::to_underlying(leaf::message_type::file_block_response))
    {
        on_file_block_response(leaf::message::deserialize_file_block_response(r));
    }
    if (type == leaf::to_underlying(leaf::message_type::block_data_response))
    {
        on_block_data_response(leaf::message::deserialize_block_data_response(r));
    }
    if (type == leaf::to_underlying(leaf::message_type::keepalive))
    {
        on_keepalive_response(leaf::message::deserialize_keepalive_response(r));
    }
    if (type == leaf::to_underlying(leaf::message_type::error))
    {
        on_error_response(leaf::message::deserialize_error_response(r));
    }
    if (type == leaf::to_underlying(leaf::message_type::login_response))
    {
        on_login_response(leaf::message::deserialize_login_response(r));
    }
}
void download_session::on_login_response(const std::optional<leaf::login_response>& message)
{
    if (!message.has_value())
    {
        return;
    }

    const auto& l = message.value();
    assert(token_.token == l.token);
    login_ = true;
    LOG_INFO("{} login_response user {} token {}", id_, l.username, l.token);
    leaf::notify_event e;
    e.method = "login";
    e.data = true;
    notify_cb_(e);
}

void download_session::download_file_request()
{
    if (file_ == nullptr)
    {
        return;
    }
    leaf::download_file_request req;
    req.filename = file_->filename;
    LOG_INFO("{} download_file_request {}", id_, file_->file_path);
    write_message(req);
}

void download_session::on_download_file_response(const std::optional<leaf::download_file_response>& message)
{
    if (!message.has_value())
    {
        return;
    }

    const auto& msg = message.value();

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
        reset();
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

void download_session::on_file_block_response(const std::optional<leaf::file_block_response>& message)
{
    if (!message.has_value())
    {
        return;
    }

    const auto& msg = message.value();

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

void download_session::on_block_data_response(const std::optional<leaf::block_data_response>& message)
{
    if (!message.has_value())
    {
        return;
    }

    const auto& msg = message.value();

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
void download_session::on_block_data_finish(const std::optional<leaf::block_data_finish>& message)
{
    if (!message.has_value())
    {
        return;
    }

    const auto& msg = message.value();

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
    this->reset();
}

void download_session::reset()
{
    if (file_)
    {
        file_.reset();
    }
    if (writer_)
    {
        writer_.reset();
    }
    if (hash_)
    {
        hash_.reset();
    }
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
    keepalive();
    update_download_file();
    download_file_request();
}

void download_session::on_error_response(const std::optional<leaf::error_response>& message)
{
    if (!message.has_value())
    {
        return;
    }

    const auto& msg = message.value();

    if (msg.error == boost::system::errc::no_such_file_or_directory)
    {
        leaf::notify_event e;
        e.method = "file_not_exist";
        e.data = file_->filename;
        file_.reset();
        notify_cb_(e);
    }
    LOG_ERROR("{} error {} {}", id_, msg.error, msg.message);
    this->reset();
}

void download_session::on_keepalive_response(const std::optional<leaf::keepalive>& message)
{
    if (!message.has_value())
    {
        return;
    }

    const auto& k = message.value();

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
    k.token = token_.token;
    write_message(k);
}

}    // namespace leaf
