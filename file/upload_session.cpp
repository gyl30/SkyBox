#include <utility>
#include <filesystem>
#include "log/log.h"
#include "file/file.h"
#include "file/hash_file.h"
#include "file/upload_session.h"

namespace leaf
{
upload_session::upload_session(std::string id, leaf::upload_progress_callback cb)
    : id_(std::move(id)), progress_cb_(std::move(cb))
{
}

upload_session::~upload_session() = default;

void upload_session::startup() { LOG_INFO("{} startup", id_); }

void upload_session::shutdown() { LOG_INFO("{} shutdown", id_); }

void upload_session::login(const std::string& user, const std::string& pass, const leaf::login_token& l)
{
    LOG_INFO("{} login user {} pass {}", id_, user, pass);
    leaf::login_request req;
    req.username = user;
    req.password = pass;
    req.token = l.token;
    token_ = l;
    write_message(req);
}

void upload_session::set_message_cb(std::function<void(std::vector<uint8_t>)> cb) { cb_ = std::move(cb); }

void upload_session::on_message(const std::vector<uint8_t>& bytes)
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
    if (type == leaf::to_underlying(leaf::message_type::upload_file_response))
    {
        on_upload_file_response(leaf::message::deserialize_upload_file_response(r));
    }
    if (type == leaf::to_underlying(leaf::message_type::delete_file_response))
    {
        on_delete_file_response(leaf::message::deserialize_delete_file_response(r));
    }
    if (type == leaf::to_underlying(leaf::message_type::block_data_finish))
    {
        on_block_data_finish(leaf::message::deserialize_block_data_finish(r));
    }
    if (type == leaf::to_underlying(leaf::message_type::upload_file_exist))
    {
        on_upload_file_exist(leaf::message::deserialize_upload_file_exist(r));
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
    if (type == leaf::to_underlying(leaf::message_type::block_data_request))
    {
        on_block_data_request(leaf::message::deserialize_block_data_request(r));
    }
}

void upload_session::on_upload_file_response(const std::optional<leaf::upload_file_response>& message)
{
    if (!message.has_value())
    {
        return;
    }
    const auto& msg = message.value();

    assert(file_ && file_->filename == msg.filename);
    file_->id = msg.file_id;
    file_->block_size = msg.block_size;
    LOG_INFO("{} upload_file_response id {} name {}", id_, msg.file_id, msg.filename);
}
void upload_session::on_delete_file_response(const std::optional<leaf::delete_file_response>& message)
{
    if (!message.has_value())
    {
        return;
    }
    const auto& msg = message.value();

    LOG_INFO("{} delete_file_response name {}", id_, msg.filename);
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
    leaf::upload_file_request create;
    create.file_size = file_size;
    create.hash = h;
    create.filename = std::filesystem::path(file_->file_path).filename().string();
    file_->file_size = file_size;
    file_->filename = create.filename;
    write_message(create);
}
void upload_session::write_message(const codec_message& msg)
{
    if (cb_)
    {
        auto bytes = leaf::serialize_message(msg);
        cb_(std::move(bytes));
    }
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

void upload_session::on_block_data_request(const std::optional<leaf::block_data_request>& message)
{
    if (!message.has_value())
    {
        return;
    }
    const auto& msg = message.value();

    assert(file_ && reader_ && blake2b_);
    LOG_DEBUG("{} block_data_request id {} block {}", id_, msg.file_id, msg.block_id);
    if (msg.block_id != file_->active_block_count)
    {
        LOG_ERROR(
            "{} block_data_request block {} less than send block {}", id_, msg.block_id, file_->active_block_count);
        return;
    }

    boost::system::error_code ec;
    std::vector<uint8_t> buffer(file_->block_size, '0');
    auto read_size = reader_->read(buffer.data(), buffer.size(), ec);
    if (ec)
    {
        LOG_ERROR("{} block_data_request {} error {}", id_, msg.block_id, ec.message());
        return;
    }
    blake2b_->update(buffer.data(), read_size);
    buffer.resize(read_size);
    file_->active_block_count = msg.block_id;
    LOG_DEBUG("{} block_data_request id {} block {} size {}", id_, msg.file_id, msg.block_id, read_size);
    leaf::block_data_response response;
    response.file_id = msg.file_id;
    response.block_id = msg.block_id;
    response.data = std::move(buffer);
    write_message(response);
    file_->active_block_count = msg.block_id + 1;
    upload_event e;
    e.file_size = file_->file_size;
    e.upload_size = reader_->size();
    e.filename = file_->filename;
    e.id = file_->id;
    emit_event(e);
}

void upload_session::emit_event(const leaf::upload_event& e)
{
    if (progress_cb_)
    {
        progress_cb_(e);
    }
}

void upload_session::on_block_data_finish(const std::optional<leaf::block_data_finish>& message)
{
    if (!message.has_value())
    {
        return;
    }
    const auto& msg = message.value();

    assert(file_ && reader_ && blake2b_);
    blake2b_->final();
    LOG_INFO("{} block_data_finish id {} file {} hash {} size {} read size {} read hash {}",
             id_,
             msg.file_id,
             msg.filename,
             msg.hash,
             file_->file_size,
             reader_->size(),
             blake2b_->hex());

    file_ = nullptr;
    blake2b_.reset();
    auto r = reader_;
    reader_ = nullptr;
    auto ec = r->close();
    if (ec)
    {
        LOG_ERROR("{} close file error {}", id_, ec.message());
        return;
    }
}
void upload_session::on_upload_file_exist(const std::optional<leaf::upload_file_exist>& message)
{
    if (!message.has_value())
    {
        return;
    }
    const auto& msg = message.value();

    assert(file_ && reader_ && blake2b_);
    auto ec = reader_->close();
    if (ec)
    {
        LOG_ERROR("{} close file error {}", id_, ec.message());
        return;
    }
    reader_.reset();
    blake2b_->final();
    blake2b_.reset();

    std::string filename = std::filesystem::path(file_->file_path).filename().string();
    assert(filename == msg.filename);
    file_ = nullptr;
    LOG_INFO("{} upload_file_exist {} hash {}", id_, msg.filename, msg.hash);
}

void upload_session::on_error_response(const std::optional<leaf::error_response>& message)
{
    if (!message.has_value())
    {
        return;
    }
    const auto& msg = message.value();

    LOG_ERROR("{} error {}", id_, msg.error);
}

void upload_session::on_login_response(const std::optional<leaf::login_response>& message)
{
    if (!message.has_value())
    {
        return;
    }
    const auto& msg = message.value();

    login_ = true;
    assert(token_.token == msg.token);
    LOG_INFO("{} login_response user {} token {}", id_, msg.username, msg.token);
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
              msg.token);
}

void upload_session::padding_file_event()
{
    for (const auto& p : padding_files_)
    {
        upload_event e;
        e.file_size = p->file_size;
        e.upload_size = 0;
        e.filename = p->filename;
        e.id = p->id;
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
    k.token = token_.token;
    write_message(k);
}

}    // namespace leaf
