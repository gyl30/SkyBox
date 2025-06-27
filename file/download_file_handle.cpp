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
download_file_handle::download_file_handle(const boost::asio::any_io_executor& io, std::string id, leaf::websocket_session::ptr& session)
    : id_(std::move(id)), session_(session), io_(io)
{
    LOG_INFO("create {}", id_);
}

download_file_handle::~download_file_handle() { LOG_INFO("destroy {}", id_); }

void download_file_handle::startup()
{
    LOG_INFO("startup {}", id_);

    boost::asio::co_spawn(io_, [this, self = shared_from_this()]() -> boost::asio::awaitable<void> { co_await loop(); }, boost::asio::detached);
}

boost::asio::awaitable<void> download_file_handle ::write(const std::vector<uint8_t>& msg, boost::beast::error_code& ec)
{
    co_await session_->write(ec, msg.data(), msg.size());
}

void download_file_handle ::shutdown()
{
    boost::asio::co_spawn(
        io_, [this, self = shared_from_this()]() -> boost::asio::awaitable<void> { co_await shutdown_coro(); }, boost::asio::detached);
}

boost::asio::awaitable<void> download_file_handle::shutdown_coro()
{
    if (session_)
    {
        session_->close();
        session_.reset();
    }
    LOG_INFO("{} shutdown", id_);
    co_return;
}
boost::asio::awaitable<void> download_file_handle::loop()
{
    auto self = shared_from_this();
    LOG_INFO("{} recv startup", id_);
    boost::beast::error_code ec;
    co_await session_->handshake(ec);
    if (ec)
    {
        LOG_ERROR("{} handshake error {}", id_, ec.message());
        co_return;
    }
    LOG_INFO("{} handshake success", id_);

    // setup 1 wait login
    LOG_INFO("{} wait login", id_);
    co_await wait_login(ec);
    if (ec)
    {
        LOG_ERROR("{} login error {}", id_, ec.message());
        co_return;
    }
    LOG_INFO("{} login success token {}", id_, token_);
    boost::beast::flat_buffer buffer;
    while (true)
    {
        auto k = co_await wait_keepalive(ec);
        if (ec)
        {
            break;
        }
        if (k.server_timestamp == 0)
        {
            continue;    // ignore keepalive with server_timestamp 0
        }
        // setup 2 wait download file request
        LOG_INFO("{} wait download file request", id_);
        auto ctx = co_await wait_download_file_request(ec);
        if (ec)
        {
            LOG_ERROR("{} wait download file request error {}", id_, ec.message());
            break;
        }

        LOG_INFO("{} download file request {} size {}", id_, ctx.file->local_path, ctx.file->file_size);
        // setup 3 send ack
        LOG_INFO("{} send ack {}", id_, ctx.file->local_path);
        co_await send_ack(ec);
        if (ec)
        {
            LOG_ERROR("{} ack error {} {}", id_, ctx.file->local_path, ec.message());
            break;
        }
        LOG_INFO("{} ack success {}", id_, ctx.file->local_path);

        LOG_INFO("{} send file data {}", id_, ctx.file->local_path);
        // setup 4 send file data
        co_await send_file_data(ctx, ec);
        if (ec)
        {
            LOG_ERROR("{} file data error {} {}", id_, ctx.file->local_path, ec.message());
            break;
        }
        LOG_INFO("{} send file data {} complete", id_, ctx.file->local_path);
        // setup 5 send data done
        LOG_INFO("{} send file done {}", id_, ctx.file->local_path);
        co_await send_file_done(ec);
        if (ec)
        {
            LOG_ERROR("{} file done error {} {}", id_, ctx.file->local_path, ec.message());
            break;
        }
        LOG_INFO("{} send file done {} complete", id_, ctx.file->local_path);
    }
    LOG_INFO("{} recv shutdown", id_);
}
boost::asio::awaitable<void> download_file_handle::send_ack(boost::beast::error_code& ec)
{
    leaf::ack a;
    co_await write(leaf::serialize_ack(a), ec);
}
boost::asio::awaitable<void> download_file_handle::wait_login(boost::beast::error_code& ec)
{
    boost::beast::flat_buffer buffer;
    co_await session_->read(ec, buffer);
    if (ec)
    {
        co_return;
    }
    auto message = boost::beast::buffers_to_string(buffer.data());
    auto type = leaf::get_message_type(message);
    auto bytes = std::vector<uint8_t>(message.begin(), message.end());
    if (type != leaf::message_type::login)
    {
        ec = boost::system::errc::make_error_code(boost::system::errc::protocol_error);
        co_return;
    }
    auto login = leaf::deserialize_login_token(bytes);
    if (!login.has_value())
    {
        ec = boost::system::errc::make_error_code(boost::system::errc::protocol_error);
        co_return;
    }

    token_ = login->token;
    co_await write(leaf::serialize_login_token(login.value()), ec);
}
boost::asio::awaitable<leaf::keepalive> download_file_handle::wait_keepalive(boost::beast::error_code& ec)
{
    leaf::keepalive k;
    boost::beast::flat_buffer buffer;
    co_await session_->read(ec, buffer);
    if (ec)
    {
        co_return k;
    }
    auto message = boost::beast::buffers_to_string(buffer.data());
    auto type = leaf::get_message_type(message);
    if (type != leaf::message_type::keepalive)
    {
        ec = boost::system::errc::make_error_code(boost::system::errc::protocol_error);
        co_return k;
    }
    auto keepalive_request = leaf::deserialize_keepalive(std::vector<uint8_t>(message.begin(), message.end()));
    if (!keepalive_request.has_value())
    {
        ec = boost::system::errc::make_error_code(boost::system::errc::protocol_error);
        co_return k;
    }

    k = keepalive_request.value();

    k.server_timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    LOG_DEBUG("{} on_keepalive client {} server_timestamp {} client_timestamp {} token {}",
              id_,
              k.client_id,
              k.server_timestamp,
              k.client_timestamp,
              token_);
    co_await write(serialize_keepalive(k), ec);
    co_return keepalive_request.value();
}
boost::asio::awaitable<void> download_file_handle::error_message(uint32_t id, int32_t error_code)
{
    boost::system::error_code ec;
    leaf::error_message e;
    e.id = id;
    e.error = error_code;
    co_await write(leaf::serialize_error_message(e), ec);
}

boost::asio::awaitable<void> download_file_handle::send_file_data(leaf::download_file_handle::download_context& ctx, boost::beast::error_code& ec)
{
    uint8_t buffer[kBlockSize] = {0};
    auto hash = std::make_shared<leaf::blake2b>();
    auto reader = std::make_shared<leaf::file_reader>(ctx.file->local_path);
    ec = reader->open();
    if (ec)
    {
        LOG_ERROR("{} download file open file {} error {}", id_, ctx.file->local_path, ec.message());
        co_return;
    }
    while (true)
    {
        auto read_size = reader->read_at(static_cast<int64_t>(reader->size()), buffer, kBlockSize, ec);
        if (ec && ec != boost::asio::error::eof)
        {
            LOG_ERROR("{} download file read file {} error {}", id_, ctx.file->local_path, ec.message());
            break;
        }
        leaf::file_data fd;
        fd.data.insert(fd.data.end(), buffer, buffer + read_size);
        if (read_size != 0)
        {
            hash->update(fd.data.data(), read_size);
            ctx.file->hash_count++;
        }
        // block count hash or eof hash
        if (ctx.file->hash_count == kHashBlockCount || ctx.file->file_size == reader->size() || ec == boost::asio::error::eof)
        {
            hash->final();
            fd.hash = hash->hex();
            ctx.file->hash_count = 0;
            hash = std::make_shared<leaf::blake2b>();
        }
        LOG_DEBUG("{} download file {} size {} hash {}", id_, ctx.file->local_path, read_size, fd.hash.empty() ? "empty" : fd.hash);
        boost::beast::error_code write_ec;
        if (!fd.data.empty())
        {
            co_await write(leaf::serialize_file_data(fd), write_ec);
        }
        if (ec == boost::asio::error::eof || reader->size() == ctx.file->file_size)
        {
            LOG_INFO("{} download file {} complete", id_, ctx.file->local_path);
            break;
        }
        ec = write_ec;
        if (ec)
        {
            break;
        }
    }
    ec = reader->close();
    if (ec)
    {
        LOG_ERROR("{} download file close file {} error {}", id_, ctx.file->local_path, ec.message());
    }
}

boost::asio::awaitable<void> download_file_handle::send_file_done(boost::beast::error_code& ec)
{
    leaf::done d;
    co_await write(leaf::serialize_done(d), ec);
}

boost::asio::awaitable<leaf::download_file_handle::download_context> download_file_handle::wait_download_file_request(boost::beast::error_code& ec)
{
    leaf::download_file_handle::download_context ctx;
    boost::beast::flat_buffer buffer;
    co_await session_->read(ec, buffer);
    if (ec)
    {
        co_return ctx;
    }
    auto message = boost::beast::buffers_to_string(buffer.data());
    auto type = leaf::get_message_type(message);
    if (type != leaf::message_type::download_file_request)
    {
        ec = boost::system::errc::make_error_code(boost::system::errc::protocol_error);
        co_return ctx;
    }
    auto download = leaf::deserialize_download_file_request(std::vector<uint8_t>(message.begin(), message.end()));
    if (!download.has_value())
    {
        ec = boost::system::errc::make_error_code(boost::system::errc::protocol_error);
        co_return ctx;
    }
    const auto& msg = download.value();
    auto download_file_path = leaf::encode_leaf_filename(leaf::make_file_path(token_, leaf::encode(msg.filename)));
    LOG_INFO("{} download file {} to {}", id_, msg.filename, download_file_path);
    bool exist = std::filesystem::exists(download_file_path, ec);
    if (ec)
    {
        LOG_ERROR("{} download file {} exist error {}", id_, msg.filename, ec.message());
        co_return ctx;
    }
    if (!exist)
    {
        LOG_ERROR("{} download file {} not exist", id_, download_file_path);
        ec = boost::system::errc::make_error_code(boost::system::errc::no_such_file_or_directory);
        co_return ctx;
    }

    auto file_size = std::filesystem::file_size(download_file_path, ec);
    if (ec)
    {
        LOG_ERROR("{} download file {} size error {}", id_, msg.filename, ec.message());
        co_return ctx;
    }
    auto file = std::make_shared<leaf::file_info>();
    file->local_path = download_file_path;
    file->file_size = file_size;
    file->filename = msg.filename;
    ctx.file = file;
    ctx.request = download.value();
    leaf::download_file_response response;
    response.filename = file->filename;
    response.id = download->id;
    response.filesize = file->file_size;
    co_await write(leaf::serialize_download_file_response(response), ec);
    co_return ctx;
}

}    // namespace leaf
