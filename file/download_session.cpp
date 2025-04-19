#include <utility>
#include <string>
#include <filesystem>
#include "log/log.h"
#include "file/file.h"
#include "config/config.h"
#include "protocol/codec.h"
#include "file/download_session.h"

namespace leaf
{
download_session::download_session(std::string id,
                                   std::string token,
                                   leaf::download_progress_callback cb,
                                   leaf::notify_progress_callback notify_cb,
                                   boost::asio::ip::tcp::endpoint ed_,
                                   boost::asio::io_context& io)
    : id_(std::move(id)),
      token_(std::move(token)),
      io_(io),
      ed_(std::move(ed_)),
      notify_cb_(std::move(notify_cb)),
      progress_cb_(std::move(cb))
{
}

download_session::~download_session() = default;

void download_session::startup()
{
    std::string url = "ws://" + ed_.address().to_string() + ":" + std::to_string(ed_.port()) + "/leaf/ws/download";
    ws_client_ = std::make_shared<leaf::plain_websocket_client>(id_, url, ed_, io_);
    ws_client_->set_read_cb([this, self = shared_from_this()](auto ec, const auto& msg) { on_read(ec, msg); });
    ws_client_->set_write_cb([this, self = shared_from_this()](auto ec, std::size_t bytes) { on_write(ec, bytes); });
    ws_client_->set_handshake_cb([this, self = shared_from_this()](auto ec) { on_connect(ec); });
    ws_client_->startup();
    LOG_INFO("{} startup", id_);
}

void download_session::shutdown()
{
    if (ws_client_)
    {
        ws_client_->shutdown();
        ws_client_.reset();
    }

    LOG_INFO("{} shutdown", id_);
}

void download_session::on_connect(boost::beast::error_code ec)
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
void download_session::on_read(boost::beast::error_code ec, const std::vector<uint8_t>& bytes)
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
    if (type == leaf::message_type::login)
    {
        on_login_token(leaf::deserialize_login_token(bytes));
        return;
    }
    if (type == leaf::message_type::download_file_response)
    {
        on_download_file_response(leaf::deserialize_download_file_response(bytes));
        return;
    }
    if (type == leaf::message_type::file_data)
    {
        on_file_data(leaf::deserialize_file_data(bytes));
        return;
    }
    if (type == leaf::message_type::keepalive)
    {
        on_keepalive_response(leaf::deserialize_keepalive_response(bytes));
        return;
    }
}
void download_session::on_write(boost::beast::error_code ec, std::size_t /*transferred*/)
{
    if (ec)
    {
        shutdown();
        return;
    }
}
void download_session::on_login_token(const std::optional<leaf::login_token>& message)
{
    if (!message.has_value())
    {
        return;
    }

    const auto& l = message.value();
    assert(token_ == l.token);
    login_ = true;
    LOG_INFO("{} login_token {}", id_, l.token);
    leaf::notify_event e;
    e.method = "login";
    e.data = true;
    notify_cb_(e);
}

void download_session::download_file_request()
{
    if (status_ != wait_download_file)
    {
        return;
    }
    if (file_ != nullptr)
    {
        return;
    }

    if (padding_files_.empty())
    {
        LOG_TRACE("{} padding files empty", id_);
        return;
    }
    std::string filename = padding_files_.front();
    padding_files_.pop();
    leaf::download_file_request req;
    req.filename = filename;
    req.id = ++seq_;
    LOG_INFO("{} download_file {}", id_, req.filename);
    ws_client_->write(serialize_download_file_request(req));
}

void download_session::on_download_file_response(const std::optional<leaf::download_file_response>& res)
{
    status_ = wait_download_file;
    if (!res.has_value())
    {
        return;
    }

    const auto& msg = res.value();
    boost::system::error_code exists_ec;
    auto file_path = std::filesystem::path(kDefatuleDownloadDir).append(msg.filename).string();
    bool exists = std::filesystem::exists(file_path, exists_ec);
    if (exists_ec)
    {
        LOG_ERROR("{} download_file file {} exists error {}", id_, file_path, exists_ec.message());
        reset_state();
        return;
    }
    if (exists)
    {
        uint32_t exists_size = std::filesystem::file_size(file_path, exists_ec);
        if (exists_ec)
        {
            LOG_ERROR("{} download_file {} file size failed {}", id_, file_path, exists_ec.message());
            reset_state();
            return;
        }
        if (exists_size != res->filesize)
        {
            LOG_INFO("{} download_file {} file size not match {}:{}", id_, file_path, exists_size, res->filesize);
            reset_state();
            return;
        }
    }

    assert(!file_ && !writer_ && !hash_);

    writer_ = std::make_shared<leaf::file_writer>(file_path);
    auto ec = writer_->open();
    if (ec)
    {
        LOG_ERROR("{} download_file open file {} error {}", id_, file_path, ec.message());
        return;
    }

    hash_ = std::make_shared<leaf::blake2b>();
    file_ = std::make_shared<leaf::file_context>();
    file_->filename = msg.filename;
    file_->file_size = res->filesize;
    file_->file_path = file_path;
    writer_ = std::make_shared<leaf::file_writer>(file_->file_path);
    ec = writer_->open();
    if (ec)
    {
        LOG_ERROR("{} upload_file open file {} error {}", id_, file_path, ec.message());
        reset_state();
        return;
    }

    LOG_INFO("{} download_file {} file size {}", id_, file_path, file_->file_size);
    status_ = wait_file_data;
}

void download_session::on_file_data(const std::optional<leaf::file_data>& data)
{
    assert(status_ == wait_file_data);
    if (!data.has_value())
    {
        return;
    }
    assert(data->data.size() <= kBlockSize);
    assert(writer_->size() <= file_->file_size);
    boost::system::error_code ec;
    writer_->write(data->data.data(), data->data.size(), ec);
    if (ec)
    {
        LOG_ERROR("{} download_file {} write error {}", id_, file_->filename, ec.message());
        reset_state();
        return;
    }
    file_->hash_count++;
    hash_->update(data->data.data(), data->data.size());
    LOG_DEBUG("{} download_file {} hash count {} hash {} data size {} write size {}",
              id_,
              file_->file_path,
              file_->hash_count,
              data->hash.empty() ? "empty" : data->hash,
              data->data.size(),
              writer_->size());

    if (!data->hash.empty())
    {
        hash_->final();
        auto hash = hash_->hex();
        if (hash != data->hash)
        {
            LOG_ERROR("{} download_file {} hash not match {} {}", id_, file_->file_path, hash, data->hash);
            reset_state();
            return;
        }
        file_->hash_count = 0;
        hash_ = std::make_shared<leaf::blake2b>();
    }
    download_event d;
    d.filename = file_->file_path;
    d.download_size = writer_->size();
    d.file_size = file_->file_size;
    emit_event(d);
    if (file_->file_size == writer_->size())
    {
        LOG_INFO("{} download_file {} size {} done", id_, file_->file_path, d.file_size);
        reset_state();
        return;
    }
}
void download_session::emit_event(const leaf::download_event& e)
{
    if (progress_cb_)
    {
        progress_cb_(e);
    }
}

void download_session::reset_state()
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
    status_ = wait_download_file;
}
void download_session::add_file(const std::string& file) { padding_files_.push(file); }

void download_session::update()
{
    if (!login_)
    {
        return;
    }
    // keepalive();
    download_file_request();
}

void download_session::on_error_message(const std::optional<leaf::error_message>& message)
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
    LOG_ERROR("{} download_file error {} {}", id_, msg.id, msg.error);
    this->reset_state();
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
              token_);
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
    ws_client_->write(leaf::serialize_keepalive(k));
}

}    // namespace leaf
