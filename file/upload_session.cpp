#include <utility>
#include "log/log.h"
#include "file/file.h"
#include "file/event.h"
#include "config/config.h"
#include "crypt/blake2b.h"
#include "protocol/codec.h"
#include "protocol/message.h"
#include "file/event_manager.h"
#include "file/upload_session.h"

namespace leaf
{
upload_session::upload_session(std::string id, std::string host, std::string port, std::string token, boost::asio::io_context& io)
    : id_(std::move(id)), host_(std::move(host)), port_(std::move(port)), token_(std::move(token)), io_(io)
{
}

upload_session::~upload_session() = default;

void upload_session::startup()
{
    LOG_INFO("{} startup", id_);
    ws_client_ = std::make_shared<leaf::plain_websocket_client>(id_, host_, port_, "/leaf/ws/upload", io_);
    boost::asio::co_spawn(io_, [this, self = shared_from_this()]() -> boost::asio::awaitable<void> { co_await loop(); }, boost::asio::detached);
}

boost::asio::awaitable<void> upload_session::login(boost::beast::error_code& ec)
{
    LOG_INFO("{} connect ws client will login use token {}", id_, token_);
    leaf::login_token lt;
    lt.id = 0x01;
    lt.token = token_;
    co_await write(leaf::serialize_login_token(lt), ec);
    if (ec)
    {
        co_return;
    }
    boost::beast::flat_buffer buffer;
    co_await ws_client_->read(ec, buffer);
    if (ec)
    {
        LOG_ERROR("{} login error {}", id_, ec.message());
        co_return;
    }

    auto message = boost::beast::buffers_to_string(buffer.data());
    auto type = leaf::get_message_type(message);
    auto bytes = std::vector<uint8_t>(message.begin(), message.end());
    if (type != leaf::message_type::login)
    {
        LOG_ERROR("{} message type error login:{}", id_, leaf::message_type_to_string(type));
        ec = boost::system::errc::make_error_code(boost::system::errc::protocol_error);
        co_return;
    }
    auto login = leaf::deserialize_login_token(bytes);
    if (!login.has_value())
    {
        LOG_ERROR("{} deserialize login message error", id_);
        ec = boost::system::errc::make_error_code(boost::system::errc::protocol_error);
        co_return;
    }
}
boost::asio::awaitable<void> upload_session::loop()
{
    LOG_INFO("{} startup", id_);
    boost::beast::error_code ec;
    co_await ws_client_->handshake(ec);
    if (ec)
    {
        LOG_ERROR("{} handshake error {}", id_, ec.message());
        co_return;
    }
    LOG_INFO("{} handshake success", id_);
    co_await login(ec);
    if (ec)
    {
        LOG_ERROR("{} login error {}", id_, ec.message());
        co_return;
    }
    LOG_INFO("{} login success token {}", id_, token_);
    while (true)
    {
        co_await send_keepalive(ec);
        if (ec)
        {
            break;
        }
        if (padding_files_.empty())
        {
            co_await delay(3);
            continue;
        }
        auto file = padding_files_.front();
        padding_files_.pop_front();
        // send upload file request
        LOG_INFO("{} send upload request file {} name {} dir {}", id_, file.local_path, file.filename, file.dir);
        co_await send_upload_file_request(file, ec);
        if (ec)
        {
            LOG_ERROR("{} send upload file request error {} {}", id_, ec.message(), file.local_path);
            break;
        }
        // wait upload file response
        LOG_INFO("{} wait upload file response {}", id_, file.local_path);
        co_await wait_upload_file_response(ec);
        if (ec)
        {
            LOG_ERROR("{} wait upload file response error {} {}", id_, ec.message(), file.local_path);
            break;
        }
        // send ack
        LOG_INFO("{} send ack {}", id_, file.local_path);
        co_await send_ack(ec);
        if (ec)
        {
            LOG_ERROR("{} send ack error {} {}", id_, ec.message(), file.local_path);
            break;
        }
        // send file data
        LOG_INFO("{} send file data {}", id_, file.local_path);
        co_await send_file_data(file, ec);
        if (ec)
        {
            LOG_ERROR("{} send file data error {} {}", id_, ec.message(), file.local_path);
            break;
        }
        LOG_INFO("{} send file data complete {}", id_, file.local_path);
        LOG_INFO("{} wait file done {}", id_, file.local_path);
        co_await wait_file_done(ec);
        if (ec)
        {
            LOG_ERROR("{} wait file done error {} {}", id_, ec.message(), file.local_path);
            break;
        }
        LOG_INFO("{} wait file done complete {}", id_, file.local_path);
    }
    LOG_INFO("{} shutdown", id_);
}

boost::asio::awaitable<void> upload_session::write(const std::vector<uint8_t>& data, boost::system::error_code& ec)
{
    co_await ws_client_->write(ec, data.data(), data.size());
}

boost::asio::awaitable<void> upload_session::delay(int second)
{
    boost::beast::error_code ec;
    boost::asio::steady_timer timer(io_, std::chrono::seconds(second));
    co_await timer.async_wait(boost::asio::redirect_error(boost::asio::use_awaitable, ec));
}
boost::asio::awaitable<void> upload_session::shutdown_coro()
{
    if (ws_client_)
    {
        ws_client_->close();
        ws_client_.reset();
    }
    LOG_INFO("{} shutdown", id_);
    co_return;
}

void upload_session::shutdown()
{
    boost::asio::co_spawn(
        io_, [this, self = shared_from_this()]() -> boost::asio::awaitable<void> { co_await shutdown_coro(); }, boost::asio::detached);
}
void upload_session::safe_add_file(const file_info& file)
{
    LOG_INFO("{} add file {}", id_, file.filename);
    padding_files_.push_back(file);
}
void upload_session::safe_add_files(const std::vector<file_info>& files)
{
    for (const auto& filename : files)
    {
        padding_files_.push_back(filename);
    }
}

void upload_session::add_file(const file_info& file)
{
    boost::asio::post(io_, [this, file, self = shared_from_this()]() { safe_add_file(file); });
}

void upload_session::add_files(const std::vector<file_info>& files)
{
    boost::asio::post(io_, [this, files, self = shared_from_this()]() { safe_add_files(files); });
}

boost::asio::awaitable<void> upload_session::send_upload_file_request(const file_info& file, boost::beast::error_code& ec)
{
    leaf::upload_file_request u;
    u.id = seq_++;
    u.dir = file.dir;
    u.filename = file.filename;
    u.filesize = file.file_size;
    LOG_DEBUG("{} upload request {} dir {} filename {} filesize {}", id_, u.id, file.dir, file.local_path, file.file_size);
    co_await write(leaf::serialize_upload_file_request(u), ec);
}

boost::asio::awaitable<void> upload_session::send_ack(boost::beast::error_code& ec)
{
    leaf::ack a;
    co_await write(leaf::serialize_ack(a), ec);
}

boost::asio::awaitable<void> upload_session::wait_upload_file_response(boost::beast::error_code& ec)
{
    boost::beast::flat_buffer buffer;
    co_await ws_client_->read(ec, buffer);
    if (ec)
    {
        LOG_ERROR("{} wait upload file response error {}", id_, ec.message());
        co_return;
    }
    auto message = boost::beast::buffers_to_string(buffer.data());
    auto type = leaf::get_message_type(message);
    if (type != leaf::message_type::upload_file_response)
    {
        ec = boost::system::errc::make_error_code(boost::system::errc::protocol_error);
        co_return;
    }
    auto response = leaf::deserialize_upload_file_response(std::vector<uint8_t>(message.begin(), message.end()));
    if (!response.has_value())
    {
        ec = boost::system::errc::make_error_code(boost::system::errc::protocol_error);
        co_return;
    }
    LOG_DEBUG("{} upload file response {} filename {}", id_, response->id, response->filename);
}
boost::asio::awaitable<void> upload_session::send_file_data(const file_info& file, boost::beast::error_code& ec)
{
    auto reader = std::make_shared<leaf::file_reader>(file.local_path);
    if (reader == nullptr)
    {
        ec = boost::system::errc::make_error_code(boost::system::errc::not_enough_memory);
        co_return;
    }

    ec = reader->open();
    if (ec)
    {
        co_return;
    }
    auto hash = std::make_shared<leaf::blake2b>();

    uint8_t buffer[kBlockSize] = {0};
    int64_t read_offset = 0;
    while (true)
    {
        assert(reader->size() <= file.file_size);
        LOG_DEBUG("{} read file {} size {} offset {} block size {}", id_, file.local_path, reader->size(), read_offset, kBlockSize);
        auto read_size = reader->read_at(read_offset, buffer, kBlockSize, ec);
        if (ec && ec != boost::asio::error::eof)
        {
            LOG_ERROR("{} read file {} size {} offset {} block size {} error {}",
                      id_,
                      file.local_path,
                      reader->size(),
                      read_offset,
                      kBlockSize,
                      ec.message());
            break;
        }
        read_offset += static_cast<int64_t>(read_size);
        leaf::file_data fd;
        fd.data.insert(fd.data.end(), buffer, buffer + read_size);
        if (read_size != 0)
        {
            hash->update(buffer, read_size);
        }
        // block count hash or eof hash
        auto reader_size = reader->size();
        if ((reader_size != 0 && read_size % kHashBlockSize == 0) || read_size == file.file_size || ec == boost::asio::error::eof)
        {
            hash->final();
            fd.hash = hash->hex();
            hash->reset();
        }
        LOG_DEBUG("{} read file {} size {} offset {} read size {} block size {} hash {}",
                  id_,
                  file.local_path,
                  reader->size(),
                  read_offset,
                  reader_size,
                  kBlockSize,
                  fd.hash.empty() ? "empty" : fd.hash);

        upload_event u;
        u.upload_size = reader->size();
        u.file_size = file.file_size;
        u.filename = file.local_path;
        leaf::event_manager::instance().post("upload", u);

        if (!fd.data.empty())
        {
            boost::beast::error_code write_ec;
            co_await write(leaf::serialize_file_data(fd), write_ec);
            if (write_ec)
            {
                break;
            }
        }

        if (ec == boost::asio::error::eof || reader->size() == file.file_size)
        {
            ec = {};
            LOG_DEBUG("{} send file done", file.local_path);
            co_await send_file_done(ec);
            break;
        }
    }
    auto close_ec = reader->close();
    if (close_ec)
    {
        LOG_ERROR("{} reader close error {} {}", id_, close_ec.message(), file.local_path);
        co_return;
    }
    LOG_DEBUG("{} reader close success {}", id_, file.local_path);
}

boost::asio::awaitable<void> upload_session::wait_file_done(boost::beast::error_code& ec)
{
    boost::beast::flat_buffer buffer;
    co_await ws_client_->read(ec, buffer);
    if (ec)
    {
        co_return;
    }
    auto message = boost::beast::buffers_to_string(buffer.data());
    auto type = leaf::get_message_type(message);
    if (type != leaf::message_type::done)
    {
        ec = boost::system::errc::make_error_code(boost::system::errc::protocol_error);
        co_return;
    }
    upload_event u;
    u.upload_size = 0;
    u.file_size = 0;
    leaf::event_manager::instance().post("upload", u);
}
boost::asio::awaitable<void> upload_session::send_file_done(boost::beast::error_code& ec)
{
    leaf::done d;
    co_await write(leaf::serialize_done(d), ec);
}

void upload_session::padding_file_event()
{
    for (const auto& file : padding_files_)
    {
        upload_event e;
        e.filename = file.local_path;
        e.file_size = file.file_size;
        e.upload_size = 0;
        LOG_DEBUG("padding files {}", e.filename);
        leaf::event_manager::instance().post("upload", e);
    }
}
boost::asio::awaitable<void> upload_session::send_keepalive(boost::beast::error_code& ec)
{
    leaf::keepalive k;
    k.id = 0;
    k.client_id = reinterpret_cast<uintptr_t>(this);
    k.client_timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    k.server_timestamp = padding_files_.size();
    LOG_DEBUG("{} keepalive request client {} server_timestamp {} client_timestamp {} token {}",
              id_,
              k.client_id,
              k.server_timestamp,
              k.client_timestamp,
              token_);

    co_await write(leaf::serialize_keepalive(k), ec);
    boost::beast::flat_buffer buffer;
    co_await ws_client_->read(ec, buffer);
    if (ec)
    {
        co_return;
    }
    auto message = boost::beast::buffers_to_string(buffer.data());
    auto type = leaf::get_message_type(message);
    if (type != leaf::message_type::keepalive)
    {
        ec = boost::system::errc::make_error_code(boost::system::errc::protocol_error);
        co_return;
    }
    auto kk = leaf::deserialize_keepalive(std::vector<uint8_t>(message.begin(), message.end()));
    if (!kk.has_value())
    {
        ec = boost::system::errc::make_error_code(boost::system::errc::protocol_error);
        co_return;
    }
    LOG_DEBUG("{} keepalive response client {} server_timestamp {} client_timestamp {} token {}",
              id_,
              kk->client_id,
              kk->server_timestamp,
              kk->client_timestamp,
              token_);
}

}    // namespace leaf
