#include <utility>
#include <filesystem>
#include "log/log.h"
#include "file/file.h"
#include "config/config.h"
#include "protocol/codec.h"
#include "file/upload_session.h"

namespace leaf
{
upload_session::upload_session(
    std::string id, std::string host, std::string port, std::string token, leaf::upload_handle handler, boost::asio::io_context& io)
    : id_(std::move(id)), host_(std::move(host)), port_(std::move(port)), token_(std::move(token)), io_(io), handler_(std::move(handler))
{
}

upload_session::~upload_session() = default;

void upload_session::startup()
{
    LOG_INFO("{} startup", id_);
    ws_client_ = std::make_shared<leaf::plain_websocket_client>(id_, host_, port_, "/leaf/ws/upload", io_);

    boost::asio::co_spawn(
        io_, [this, self = shared_from_this()]() -> boost::asio::awaitable<void> { co_await upload_coro(); }, boost::asio::detached);
    boost::asio::co_spawn(io_, [this, self = shared_from_this()]() -> boost::asio::awaitable<void> { co_await write_coro(); }, boost::asio::detached);
}

boost::asio::awaitable<void> upload_session::upload_coro()
{
    LOG_INFO("{} upload coro startup", id_);
    boost::beast::error_code ec;
    co_await ws_client_->handshake(ec);
    if (ec)
    {
        LOG_ERROR("{} upload coro handshake error {}", id_, ec.message());
        co_return;
    }
    LOG_INFO("{} connect ws client will login use token {}", id_, token_);
    leaf::login_token lt;
    lt.id = 0x01;
    lt.token = token_;
    auto bytes = leaf::serialize_login_token(lt);
    co_await channel_.async_send(boost::system::error_code{}, bytes, boost::asio::redirect_error(boost::asio::use_awaitable, ec));
    if (ec)
    {
        LOG_ERROR("{} upload coro send login token error {}", id_, ec.message());
        co_return;
    }
    while (true)
    {
        auto file = padding_files_.front();
        padding_files_.pop_front();
        auto ctx = create_upload_context(file, ec);
        if (ec)
        {
            LOG_ERROR("{} create upload context error {}", id_, ec.message());
            break;
        }
        // send keepalive
        co_await keepalive(ec);
        if (ec)
        {
            LOG_ERROR("{} keepalive error {}", id_, ec.message());
            break;
        }
        // send upload file request
        co_await send_upload_file_request(ctx, ec);
        if (ec)
        {
            LOG_ERROR("{} send upload file request error {}", id_, ec.message());
            break;
        }
        // wait upload file response
        co_await wait_upload_file_response(ec);
        if (ec)
        {
            LOG_ERROR("{} wait upload file response error {}", id_, ec.message());
            break;
        }
        // send ack
        co_await send_ack(ec);
        if (ec)
        {
            LOG_ERROR("{} send ack error {}", id_, ec.message());
            break;
        }
        // send file data
        co_await send_file_data(ctx, ec);
        if (ec)
        {
            LOG_ERROR("{} send file data error {}", id_, ec.message());
        }
    }
    LOG_INFO("{} upload coro shutdown", id_);
}

boost::asio::awaitable<void> upload_session::write_coro()
{
    LOG_INFO("{} write coro startup", id_);
    while (true)
    {
        boost::system::error_code ec;
        auto bytes = co_await channel_.async_receive(boost::asio::redirect_error(boost::asio::use_awaitable, ec));
        if (ec)
        {
            LOG_ERROR("{} write coro error {}", id_, ec.message());
            break;
        }
        co_await ws_client_->write(ec, bytes.data(), bytes.size());
        if (ec)
        {
            LOG_ERROR("{} write coro error {}", id_, ec.message());
            break;
        }
    }
    LOG_INFO("{} write coro shutdown", id_);
}
boost::asio::awaitable<void> upload_session::shutdown_coro()
{
    if (ws_client_)
    {
        channel_.close();
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

leaf::upload_session::upload_context upload_session::create_upload_context(const std::string& filename, boost::beast::error_code& ec)
{
    auto file_size = std::filesystem::file_size(filename, ec);
    if (ec)
    {
        return {};
    }
    auto file = std::make_shared<leaf::file_info>();
    file->file_path = filename;
    file->filename = std::filesystem::path(filename).filename().string();
    file->file_size = file_size;
    leaf::upload_session::upload_context ctx;
    ctx.file = file;
    return ctx;
}

boost::asio::awaitable<void> upload_session::send_upload_file_request(leaf::upload_session::upload_context& ctx, boost::beast::error_code& ec)
{
    leaf::upload_file_request u;
    u.id = seq_++;
    u.filename = std::filesystem::path(ctx.file->file_path).filename().string();
    u.filesize = ctx.file->file_size;
    LOG_DEBUG("{} upload_file request {} filename {} filesize {}", id_, u.id, ctx.file->file_path, ctx.file->file_size);
    co_await channel_.async_send(ec, leaf::serialize_upload_file_request(u), boost::asio::redirect_error(boost::asio::use_awaitable, ec));
}

boost::asio::awaitable<void> upload_session::send_ack(boost::beast::error_code& ec)
{
    leaf::ack a;
    co_await channel_.async_send(ec, leaf::serialize_ack(a), boost::asio::redirect_error(boost::asio::use_awaitable, ec));
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
    LOG_DEBUG("{} upload_file response {} filename {}", id_, response->id, response->filename);
}
boost::asio::awaitable<void> upload_session::send_file_data(leaf::upload_session::upload_context& ctx, boost::beast::error_code& ec)
{
    auto reader = std::make_shared<leaf::file_reader>(ctx.file->file_path);
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
    while (true)
    {
        assert(reader->size() < ctx.file->file_size);
        auto read_size = reader->read_at(ctx.file->offset, buffer, kBlockSize, ec);
        if (ec && ec != boost::asio::error::eof)
        {
            LOG_ERROR("{} upload_file read file {} error {}", id_, ctx.file->file_path, ec.message());
            break;
        }
        leaf::file_data fd;
        fd.data.insert(fd.data.end(), buffer, buffer + read_size);
        if (read_size != 0)
        {
            ctx.file->hash_count++;
            ctx.file->offset += static_cast<int64_t>(read_size);
            hash->update(fd.data.data(), read_size);
        }
        // block count hash or eof hash
        if (ctx.file->file_size == reader->size() || ctx.file->hash_count == kHashBlockCount || ec == boost::asio::error::eof)
        {
            hash->final();
            fd.hash = hash->hex();
            ctx.file->hash_count = 0;
            hash = std::make_shared<leaf::blake2b>();
        }
        LOG_DEBUG("{} upload_file {} size {} hash {}", id_, ctx.file->file_path, read_size, fd.hash.empty() ? "empty" : fd.hash);
        upload_event u;
        u.upload_size = reader->size();
        u.file_size = ctx.file->file_size;
        u.filename = ctx.file->filename;
        emit_event(u);

        if (!fd.data.empty())
        {
            co_await channel_.async_send(ec, leaf::serialize_file_data(fd), boost::asio::redirect_error(boost::asio::use_awaitable, ec));
            if (ec)
            {
                break;
            }
        }

        if (ec == boost::asio::error::eof || reader->size() == ctx.file->file_size)
        {
            LOG_INFO("{} upload_file {} complete", id_, ctx.file->file_path);
            ec = {};
            co_await send_file_done(ec);
        }
    }
    ec = reader->close();
    if (ec)
    {
        LOG_ERROR("{} upload_file reader close error {}", id_, ec.message());
    }
}

boost::asio::awaitable<void> upload_session::send_file_done(boost::beast::error_code& ec)
{
    leaf::done d;
    co_await channel_.async_send(ec, leaf::serialize_done(d), boost::asio::redirect_error(boost::asio::use_awaitable, ec));
}

void upload_session::emit_event(const leaf::upload_event& e) const
{
    if (handler_.upload)
    {
        handler_.upload(e);
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
boost::asio::awaitable<void> upload_session::keepalive(boost::beast::error_code& ec)
{
    leaf::keepalive k;
    k.id = 0;
    k.client_id = reinterpret_cast<uintptr_t>(this);
    k.client_timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    k.server_timestamp = 0;
    co_await channel_.async_send(ec, leaf::serialize_keepalive(k), boost::asio::redirect_error(boost::asio::use_awaitable, ec));
    boost::beast::flat_buffer buffer;
    co_await ws_client_->read(ec, buffer);
    if (ec)
    {
        LOG_ERROR("{} wait keepalive error {}", id_, ec.message());
        co_return;
    }
    auto message = boost::beast::buffers_to_string(buffer.data());
    auto type = leaf::get_message_type(message);
    if (type != leaf::message_type::keepalive)
    {
        ec = boost::system::errc::make_error_code(boost::system::errc::protocol_error);
        co_return;
    }
    auto kk = leaf::deserialize_keepalive_response(std::vector<uint8_t>(message.begin(), message.end()));
    if (!kk.has_value())
    {
        ec = boost::system::errc::make_error_code(boost::system::errc::protocol_error);
        co_return;
    }
    LOG_DEBUG("{} on_keepalive client {} server_timestamp {} client_timestamp {} token {}",
              id_,
              kk->client_id,
              kk->server_timestamp,
              kk->client_timestamp,
              token_);
}

}    // namespace leaf
