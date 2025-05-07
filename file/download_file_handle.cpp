#include <utility>
#include <filesystem>
#include <boost/system/error_code.hpp>

#include "log/log.h"
#include "file/file.h"
#include "crypt/easy.h"
#include "config/config.h"
#include "protocol/codec.h"
#include "file/download_file_handle.h"

namespace leaf
{
download_file_handle::download_file_handle(std::string id, leaf::websocket_session::ptr& session)
    : id_(std::move(id)), session_(session)
{
    LOG_INFO("create {}", id_);
}

download_file_handle::~download_file_handle() { LOG_INFO("destroy {}", id_); }

void download_file_handle::startup()
{
    LOG_INFO("startup {}", id_);
    // clang-format off
    session_->set_read_cb([this, self = shared_from_this()](boost::beast::error_code ec, const std::vector<uint8_t>& bytes) { on_read(ec, bytes); });
    session_->set_write_cb([this, self = shared_from_this()](boost::beast::error_code ec, std::size_t bytes_transferred) { on_write(ec, bytes_transferred); });
    session_->startup();
    // clang-format on
}

void download_file_handle ::shutdown()
{
    std::call_once(shutdown_flag_, [this, self = shared_from_this()]() { safe_shutdown(); });
}

void download_file_handle::safe_shutdown()
{
    session_->shutdown();
    session_.reset();
    LOG_INFO("shutdown {}", id_);
}

void download_file_handle::on_write(boost::beast::error_code ec, std::size_t /*transferred*/)
{
    if (ec)
    {
        shutdown();
        return;
    }

    if (status_ != leaf::download_file_handle::status::wait_file_data)
    {
        return;
    }
    assert(reader_->size() < file_->file_size);
    // read block data
    std::vector<uint8_t> buffer(kBlockSize, '0');
    boost::system::error_code read_ec;
    auto read_size = reader_->read_at(static_cast<int64_t>(reader_->size()), buffer.data(), buffer.size(), read_ec);
    if (read_ec && read_ec != boost::asio::error::eof)
    {
        LOG_ERROR("{} download_file read file {} error {}", id_, file_->file_path, read_ec.message());
        reset_status();
        return;
    }
    buffer.resize(read_size);
    leaf::file_data fd;
    fd.data.swap(buffer);
    if (read_size != 0)
    {
        hash_->update(fd.data.data(), read_size);
        file_->hash_count++;
    }
    // block count hash or eof hash
    if (file_->hash_count == kHashBlockCount || file_->file_size == reader_->size() ||
        read_ec == boost::asio::error::eof)
    {
        hash_->final();
        fd.hash = hash_->hex();
        file_->hash_count = 0;
        hash_ = std::make_shared<leaf::blake2b>();
    }
    LOG_DEBUG(
        "{} download_file {} size {} hash {}", id_, file_->file_path, read_size, fd.hash.empty() ? "empty" : fd.hash);
    if (!fd.data.empty())
    {
        auto bytes = leaf::serialize_file_data(fd);
        session_->write(bytes);
    }

    // eof reset
    if (read_ec == boost::asio::error::eof || reader_->size() == file_->file_size)
    {
        LOG_INFO("{} upload_file {} complete", id_, file_->file_path);
        reset_status();
        file_done();
    }
}
void download_file_handle::error_message(uint32_t id, int32_t error_code)
{
    leaf::error_message e;
    e.id = id;
    e.error = error_code;
    session_->write(leaf::serialize_error_message(e));
}

void download_file_handle::file_done()
{
    leaf::done d;
    session_->write(leaf::serialize_done(d));
}
void download_file_handle::reset_status()
{
    if (reader_)
    {
        auto ec = reader_->close();
        boost::ignore_unused(ec);
        reader_.reset();
    }
    if (file_)
    {
        file_.reset();
    }
    if (hash_)
    {
        hash_.reset();
    }
    status_ = leaf::download_file_handle::status::wait_download_file_request;
}
void download_file_handle::on_read(boost::beast::error_code ec, const std::vector<uint8_t>& bytes)
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
    if (type == leaf::message_type::keepalive)
    {
        on_keepalive(leaf::deserialize_keepalive_response(bytes));
    }
    if (type == leaf::message_type::download_file_request)
    {
        on_download_file_request(leaf::deserialize_download_file_request(bytes));
    }
}

void download_file_handle::on_login(const std::optional<leaf::login_token>& l)
{
    if (status_ != wait_login)
    {
        LOG_ERROR("{} on_login status error", id_);
        shutdown();
        return;
    }
    if (!l.has_value())
    {
        return;
    }
    status_ = wait_download_file_request;
    token_ = l->token;
    LOG_INFO("{} on_login token {}", id_, token_);
    session_->write(leaf::serialize_login_token(l.value()));
}

void download_file_handle::on_download_file_request(const std::optional<leaf::download_file_request>& download)
{
    if (status_ != wait_download_file_request)
    {
        LOG_ERROR("{} download_file status error", id_);
        shutdown();
        return;
    }
    if (!download.has_value())
    {
        return;
    }
    const auto& msg = download.value();
    auto download_file_path = leaf::encode_normal_filename(leaf::make_file_path(token_, leaf::encode(msg.filename)));
    LOG_INFO("{} download_file file {} to {}", id_, msg.filename, download_file_path);
    boost::system::error_code exists_ec;
    bool exist = std::filesystem::exists(download_file_path, exists_ec);
    if (exists_ec)
    {
        LOG_ERROR("{} download_file file {} exist error {}", id_, msg.filename, exists_ec.message());
        reset_status();
        error_message(download->id, exists_ec.value());
        return;
    }
    if (!exist)
    {
        LOG_ERROR("{} download_file file {} not exist", id_, download_file_path);
        reset_status();
        exists_ec = boost::system::errc::make_error_code(boost::system::errc::no_such_file_or_directory);
        error_message(download->id, exists_ec.value());
        return;
    }

    std::error_code size_ec;
    auto file_size = std::filesystem::file_size(download_file_path, size_ec);
    if (size_ec)
    {
        LOG_ERROR("{} download_file file {} size error {}", id_, msg.filename, size_ec.message());
        reset_status();
        error_message(download->id, size_ec.value());
        return;
    }

    assert(!file_ && !reader_ && !hash_);
    reader_ = std::make_shared<leaf::file_reader>(download_file_path);
    auto ec = reader_->open();
    if (ec)
    {
        LOG_ERROR("{} download_file open file {} error {}", id_, download_file_path, ec.message());
        reset_status();
        error_message(download->id, ec.value());
        return;
    }
    hash_ = std::make_shared<leaf::blake2b>();
    file_ = std::make_shared<leaf::file_context>();
    file_->file_path = download_file_path;
    file_->file_size = file_size;
    file_->filename = msg.filename;
    LOG_INFO("{} download_file file {} size {}", id_, file_->file_path, file_->file_size);
    leaf::download_file_response response;
    response.filename = file_->filename;
    response.id = download->id;
    response.filesize = file_->file_size;
    status_ = leaf::download_file_handle::status::wait_file_data;
    session_->write(leaf::serialize_download_file_response(response));
}

void download_file_handle::on_keepalive(const std::optional<leaf::keepalive>& k)
{
    if (!k.has_value())
    {
        return;
    }
    auto kk = k.value();
    kk.server_timestamp =
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
            .count();
    LOG_DEBUG("{} on_keepalive client {} server_timestamp {} client_timestamp {} token {}",
              id_,
              kk.client_id,
              kk.server_timestamp,
              kk.client_timestamp,
              token_);
    session_->write(leaf::serialize_keepalive(kk));
}
void download_file_handle::on_error_message(const std::optional<leaf::error_message>& e)
{
    if (!e.has_value())
    {
        return;
    }
    LOG_INFO("{} download_file error {}", id_, e->error);
}
}    // namespace leaf
