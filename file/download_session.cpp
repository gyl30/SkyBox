#include <utility>
#include <string>
#include <filesystem>
#include "log/log.h"
#include "file/file.h"
#include "file/download_session.h"

constexpr auto kBlockSize = 128L * 1024;
constexpr auto kDefatuleDownloadDir = "";

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
    cb_(leaf::serialize_login_token(lt));
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
    write_message(serialize_download_file_request(req));
    status_ = wait_file_data;
}

void download_session::on_download_file_response(const std::optional<leaf::download_file_response>& message)
{
    status_ = wait_download_file;
    if (!message.has_value())
    {
        return;
    }

    const auto& msg = message.value();
    uint32_t file_size = kBlockSize * msg.block_count;
    if (msg.padding_size != 0)
    {
        file_size = kBlockSize - msg.padding_size;
    }

    boost::system::error_code exists_ec;
    bool exists = std::filesystem::exists(msg.filename, exists_ec);
    if (exists_ec)
    {
        LOG_ERROR("{} download_file file {} exists error {}", id_, msg.filename, exists_ec.message());
        return;
    }
    if (exists)
    {
        uint32_t exists_size = std::filesystem::file_size(msg.filename, exists_ec);
        if (exists_ec)
        {
            LOG_ERROR("{} download_file {} file size failed {}", id_, msg.filename, exists_ec.message());
            return;
        }
        if (exists_size != file_size)
        {
            LOG_INFO("{} download_file {} file size not match {}:{}", id_, msg.filename, exists_size, file_size);
            return;
        }
    }

    auto file_path = std::filesystem::path(kDefatuleDownloadDir).append(file_->filename).string();
    writer_ = std::make_shared<leaf::file_writer>(file_path);
    auto ec = writer_->open();
    if (ec)
    {
        LOG_ERROR("{} download_file open file {} error {}", id_, msg.filename, ec.message());
        return;
    }
    assert(!file_ && !writer_ && !hash_);

    hash_ = std::make_shared<leaf::blake2b>();
    file_ = std::make_shared<leaf::file_context>();
    file_->filename = msg.filename;
    file_->block_count = msg.block_count;
    file_->padding_size = msg.padding_size;
    file_->file_size = kBlockSize * msg.block_count;
    file_->file_path = file_path;
    if (msg.padding_size != 0)
    {
        file_->file_size = kBlockSize - msg.padding_size;
    }

    LOG_INFO("{} download_file {} block count {} padding size {} file size {}",
             id_,
             file_->filename,
             file_->block_count,
             file_->padding_size,
             file_->file_size);
    status_ = wait_file_data;
}

void download_session::on_file_data(const std::optional<leaf::file_data>& data)
{
    status_ = wait_download_file;
    if (!data.has_value())
    {
        return;
    }
    assert(data->block_id == file_->active_block_count);
    hash_->update(data->data.data(), data->data.size());
    boost::system::error_code ec;
    writer_->write(data->data.data(), data->data.size(), ec);
    if (ec)
    {
        LOG_ERROR("{} download_file {} write error {}", id_, file_->filename, ec.message());
        return;
    }
    if (!data->hash.empty())
    {
        hash_->final();
        auto hash = hash_->hex();
        if (hash != data->hash)
        {
            LOG_ERROR("{} download_file {} hash not match {} {}", id_, file_->filename, hash, data->hash);
            return;
        }
        hash_ = std::make_shared<leaf::blake2b>();
        file_->active_block_count++;
    }
    download_event d;
    d.filename = file_->file_path;
    d.download_size = writer_->size();
    d.file_size = file_->file_size;
    emit_event(d);
    if (file_->file_size == writer_->size())
    {
        assert(file_->active_block_count == file_->block_count);
        ec = writer_->close();
        reset();
        auto hash = hash_->hex();
        LOG_INFO("{} download_file {} finish hash {}:{}", id_, file_->filename, hash, data->hash);
        return;
    }
    status_ = wait_file_data;
}
void download_session::emit_event(const leaf::download_event& e)
{
    if (progress_cb_)
    {
        progress_cb_(e);
    }
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
void download_session::write_message(std::vector<uint8_t> bytes)
{
    if (cb_)
    {
        cb_(std::move(bytes));
    }
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
    write_message(leaf::serialize_keepalive(k));
}

}    // namespace leaf
