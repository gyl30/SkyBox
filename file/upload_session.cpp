#include <utility>
#include <filesystem>
#include "log/log.h"
#include "file/file.h"
#include "config/config.h"
#include "file/upload_session.h"

namespace leaf
{
upload_session::upload_session(std::string id,
                               std::string token,
                               leaf::upload_handle handler,
                               boost::asio::ip::tcp::endpoint ed,
                               boost::asio::io_context& io)
    : id_(std::move(id)), token_(std::move(token)), io_(io), ed_(std::move(ed)), handler_(std::move(handler))
{
}

upload_session::~upload_session() = default;

void upload_session::startup()
{
    std::string url = "ws://" + ed_.address().to_string() + ":" + std::to_string(ed_.port()) + "/leaf/ws/upload";
    ws_client_ = std::make_shared<leaf::plain_websocket_client>(id_, url, ed_, io_);
    ws_client_->set_read_cb([this, self = shared_from_this()](auto ec, const auto& msg) { on_read(ec, msg); });
    ws_client_->set_write_cb([this, self = shared_from_this()](auto ec, std::size_t bytes) { on_write(ec, bytes); });
    ws_client_->set_handshake_cb([this, self = shared_from_this()](auto ec) { on_connect(ec); });
    ws_client_->startup();
    LOG_INFO("{} startup", id_);
}

void upload_session::shutdown()
{
    io_.post([this, self = shared_from_this()]() { safe_shutdown(); });
}

void upload_session::safe_shutdown()
{
    if (ws_client_)
    {
        ws_client_->shutdown();
        ws_client_.reset();
    }

    LOG_INFO("{} shutdown", id_);
}
void upload_session::on_connect(boost::beast::error_code ec)
{
    if (ec)
    {
        shutdown();
        return;
    }
    LOG_INFO("{} connect ws client will login use token {}", id_, token_);
    leaf::login_token lt;
    lt.id = seq_++;
    lt.token = token_;
    ws_client_->write(leaf::serialize_login_token(lt));
}
void upload_session::on_read(boost::beast::error_code ec, const std::vector<uint8_t>& bytes)
{
    if (ec)
    {
        shutdown();
        return;
    }
    auto type = leaf::get_message_type(bytes);
    if (type == leaf::message_type::error)
    {
        on_error_message(leaf::deserialize_error_message(bytes));
        return;
    }
    if (type == leaf::message_type::upload_file_response)
    {
        on_upload_file_response(leaf::deserialize_upload_file_response(bytes));
        return;
    }
    if (type == leaf::message_type::login)
    {
        on_login_token(leaf::deserialize_login_token(bytes));
        return;
    }
}

void upload_session::on_write(boost::beast::error_code ec, std::size_t /*transferred*/)
{
    if (ec)
    {
        shutdown();
        return;
    }
    if (state_ == file_data)
    {
        upload_file_data();
    }
}

void upload_session::safe_add_file(const std::string& filename)
{
    LOG_INFO("{} add file {}", id_, filename);
    padding_files_.push_back(filename);
}
void upload_session::safe_add_files(const std::vector<std::string>& files)
{
    for (const auto& filename : files)
    {
        padding_files_.push_back(filename);
    }
}

void upload_session::add_file(const std::string& filename)
{
    io_.post([this, filename, self = shared_from_this()]() { safe_add_file(filename); });
}

void upload_session::add_files(const std::vector<std::string>& files)
{
    io_.post([this, files, self = shared_from_this()]() { safe_add_files(files); });
}

void upload_session::update_process_file()
{
    if (state_ != logined)
    {
        return;
    }

    if (padding_files_.empty())
    {
        LOG_TRACE("{} padding files empty", id_);
        return;
    }
    state_ = upload_request;
    std::string filename = padding_files_.front();
    padding_files_.pop_front();
    std::error_code size_ec;
    auto file_size = std::filesystem::file_size(filename, size_ec);
    if (size_ec)
    {
        LOG_ERROR("{} upload_file request file {} size error {}", id_, filename, size_ec.message());
        reset_state();
        return;
    }
    file_ = std::make_shared<leaf::file_info>();
    file_->file_path = filename;
    file_->filename = std::filesystem::path(filename).filename().string();
    file_->file_size = file_size;
    LOG_INFO("{} start_file {} padding size {}", id_, file_->file_path, padding_files_.size());
    upload_file_request();
}
void upload_session::update()
{
    if (!login_)
    {
        return;
    }
    // keepalive();
    update_process_file();
    padding_file_event();
}

void upload_session::emit_event(const leaf::upload_event& e) const
{
    if (handler_.upload)
    {
        handler_.upload(e);
    }
}

void upload_session::on_error_message(const std::optional<leaf::error_message>& e)
{
    if (!e.has_value())
    {
        return;
    }

    boost::system::error_code ec;
    ec.assign(e->error, boost::system::system_category());
    LOG_ERROR("{} upload_file error {} {}", id_, e->id, ec.message());
    reset_state();
}

void upload_session::on_login_token(const std::optional<leaf::login_token>& l)
{
    if (!l.has_value())
    {
        return;
    }
    state_ = logined;
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
              token_);
}

void upload_session::upload_file_request()
{
    assert(state_ == upload_request && file_ != nullptr);
    assert(!reader_ && !hash_);

    reader_ = std::make_shared<leaf::file_reader>(file_->file_path);
    if (reader_ == nullptr)
    {
        LOG_ERROR("{} upload_file create file {} error", id_, file_->file_path);
        reset_state();
        return;
    }

    auto ec = reader_->open();
    if (ec)
    {
        LOG_ERROR("{} upload_file open file {} error {}", id_, file_->file_path, ec.message());
        reset_state();
        return;
    }
    hash_ = std::make_shared<leaf::blake2b>();
    leaf::upload_file_request u;
    u.id = seq_++;
    u.filename = std::filesystem::path(file_->file_path).filename().string();
    u.filesize = file_->file_size;
    LOG_DEBUG("{} upload_file request {} filename {} filesize {}", id_, u.id, file_->file_path, file_->file_size);
    ws_client_->write(leaf::serialize_upload_file_request(u));
}
void upload_session::on_upload_file_response(const std::optional<leaf::upload_file_response>& res)
{
    if (!res.has_value())
    {
        return;
    }
    assert(state_ == upload_request && file_ != nullptr);
    const auto& msg = res.value();
    LOG_DEBUG("{} upload_file response {} filename {}", id_, msg.id, msg.filename);
    state_ = file_data;
    leaf::ack a;
    ws_client_->write(leaf::serialize_ack(a));
}

void upload_session::file_done()
{
    leaf::done d;
    ws_client_->write(leaf::serialize_done(d));
}

void upload_session::upload_file_data()
{
    assert(reader_->size() < file_->file_size);
    // read block data
    std::vector<uint8_t> buffer(kBlockSize, '0');
    boost::system::error_code ec;
    auto read_size = reader_->read_at(file_->offset, buffer.data(), buffer.size(), ec);
    if (ec && ec != boost::asio::error::eof)
    {
        LOG_ERROR("{} upload_file read file {} error {}", id_, file_->file_path, ec.message());
        reset_state();
        return;
    }
    buffer.resize(read_size);
    leaf::file_data fd;
    fd.data.swap(buffer);
    if (read_size != 0)
    {
        file_->hash_count++;
        file_->offset += static_cast<int64_t>(read_size);
        hash_->update(fd.data.data(), read_size);
    }
    // block count hash or eof hash
    if (file_->file_size == reader_->size() || file_->hash_count == kHashBlockCount || ec == boost::asio::error::eof)
    {
        hash_->final();
        fd.hash = hash_->hex();
        file_->hash_count = 0;
        hash_ = std::make_shared<leaf::blake2b>();
    }
    LOG_DEBUG(
        "{} upload_file {} size {} hash {}", id_, file_->file_path, read_size, fd.hash.empty() ? "empty" : fd.hash);
    upload_event u;
    u.upload_size = reader_->size();
    u.file_size = file_->file_size;
    u.filename = file_->filename;
    emit_event(u);

    if (!fd.data.empty())
    {
        ws_client_->write(leaf::serialize_file_data(fd));
    }

    // eof reset
    if (ec == boost::asio::error::eof || reader_->size() == file_->file_size)
    {
        file_done();
        LOG_INFO("{} upload_file {} complete", id_, file_->file_path);
        reset_state();
    }
}

void upload_session::reset_state()
{
    state_ = logined;
    if (file_ != nullptr)
    {
        file_.reset();
    }
    if (reader_ != nullptr)
    {
        auto ec = reader_->close();
        boost::ignore_unused(ec);
        reader_.reset();
    }
    if (hash_ != nullptr)
    {
        hash_.reset();
    }
}
void upload_session::padding_file_event()
{
    for (const auto& filename : padding_files_)
    {
        upload_event e;
        e.filename = filename;
        LOG_DEBUG("padding files {}", e.filename);
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
    ws_client_->write(leaf::serialize_keepalive(k));
}

}    // namespace leaf
