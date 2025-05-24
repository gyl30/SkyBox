#include <utility>
#include <filesystem>
#include "log/log.h"
#include "crypt/passwd.h"
#include "config/config.h"
#include "protocol/codec.h"
#include "protocol/message.h"
#include "file/upload_file_handle.h"

namespace leaf
{

upload_file_handle::upload_file_handle(const boost::asio::any_io_executor& io, std::string id, leaf::websocket_session::ptr& session)
    : id_(std::move(id)), session_(session), io_(io)
{
    LOG_INFO("create {}", id_);
    key_ = leaf::passwd_key();
}

upload_file_handle::~upload_file_handle() { LOG_INFO("destroy {}", id_); }

void upload_file_handle::startup()
{
    LOG_INFO("startup {}", id_);

    boost::asio::co_spawn(io_, [this, self = shared_from_this()]() -> boost::asio::awaitable<void> { co_await recv_coro(); }, boost::asio::detached);

    boost::asio::co_spawn(io_, [this, self = shared_from_this()]() -> boost::asio::awaitable<void> { co_await write_coro(); }, boost::asio::detached);
}

boost::asio::awaitable<void> upload_file_handle ::write_coro()
{
    LOG_INFO("{} write coro startup", id_);
    while (true)
    {
        boost::system::error_code ec;
        auto bytes = co_await channel_.async_receive(boost::asio::redirect_error(boost::asio::use_awaitable, ec));
        if (ec)
        {
            LOG_ERROR("{} write coro error {}", id_, ec.message());
            co_return;
        }
        co_await session_->write(ec, bytes.data(), bytes.size());
        if (ec)
        {
            LOG_ERROR("{} write_:oro error {}", id_, ec.message());
        }
    }
    LOG_INFO("{} write coro shutdown", id_);
}

void upload_file_handle ::shutdown()
{
    boost::asio::co_spawn(
        io_, [this, self = shared_from_this()]() -> boost::asio::awaitable<void> { co_await shutdown_coro(); }, boost::asio::detached);
}

boost::asio::awaitable<void> upload_file_handle::shutdown_coro()
{
    if (session_)
    {
        channel_.close();
        session_->close();
        session_.reset();
    }
    LOG_INFO("{} shutdown", id_);
    co_return;
}
boost::asio::awaitable<void> upload_file_handle::recv_coro()
{
    boost::beast::error_code ec;
    // setup 1 wait login
    co_await wait_login(ec);
    if (ec)
    {
        LOG_ERROR("{} login error {}", id_, ec.message());
        co_return;
    }
    boost::beast::flat_buffer buffer;
    while (true)
    {
        co_await on_keepalive(ec);
        if (ec)
        {
            LOG_ERROR("{} keepalive error {}", id_, ec.message());
            break;
        }
        continue;

        // setup 2 wait upload file request
        auto ctx = co_await wait_upload_file_request(ec);
        if (ec)
        {
            LOG_ERROR("{} upload file request error {}", id_, ec.message());
            co_return;
        }

        // setup 3 wait ack
        co_await wait_ack(ec);
        if (ec)
        {
            LOG_ERROR("{} ack error {}", id_, ec.message());
            co_return;
        }

        // setup 4 wait file data
        co_await wait_file_data(ctx, ec);
        if (ec)
        {
            LOG_ERROR("{} file data error {}", id_, ec.message());
            co_return;
        }
    }
}

boost::asio::awaitable<void> upload_file_handle::wait_login(boost::beast::error_code& ec)
{
    co_await session_->handshake(ec);
    if (ec)
    {
        LOG_ERROR("{} handshake error {}", id_, ec.message());
        co_return;
    }
    boost::beast::flat_buffer buffer;
    co_await session_->read(ec, buffer);
    if (ec)
    {
        LOG_ERROR("{} recv coro error {}", id_, ec.message());
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
    LOG_INFO("{} login success token {}", id_, token_);
    co_await channel_.async_send(ec, leaf::serialize_login_token(login.value()), boost::asio::redirect_error(boost::asio::use_awaitable, ec));
}

boost::asio::awaitable<leaf::upload_file_handle::upload_context> upload_file_handle::wait_upload_file_request(boost::beast::error_code& ec)
{
    leaf::upload_file_handle::upload_context ctx;
    boost::beast::flat_buffer buffer;
    co_await session_->read(ec, buffer);
    if (ec)
    {
        LOG_ERROR("{} upload file request error {}", id_, ec.message());
        co_return ctx;
    }
    auto message = boost::beast::buffers_to_string(buffer.data());
    buffer.consume(buffer.size());
    auto type = leaf::get_message_type(message);
    if (type != leaf::message_type::upload_file_request)
    {
        ec = boost::system::errc::make_error_code(boost::system::errc::protocol_error);
        co_return ctx;
    }
    auto bytes = std::vector<uint8_t>(message.begin(), message.end());
    auto req = leaf::deserialize_upload_file_request(bytes);
    if (!req.has_value())
    {
        ec = boost::system::errc::make_error_code(boost::system::errc::protocol_error);
        co_return ctx;
    }

    auto upload_file_path = leaf::encode_tmp_filename(leaf::make_file_path(token_, req->filename));
    bool exist = std::filesystem::exists(upload_file_path, ec);
    if (ec)
    {
        LOG_ERROR("{} upload_file request file {} exist failed {}", id_, upload_file_path, ec.message());
        co_return ctx;
    }
    if (exist)
    {
        LOG_ERROR("{} upload_file request file {} exist", id_, upload_file_path);
        ec = boost::system::errc::make_error_code(boost::system::errc::file_exists);
        co_return ctx;
    }
    LOG_INFO("{} upload_file request file size {} name {} path {}", id_, req->filesize, req->filename, upload_file_path);

    auto file = std::make_shared<file_info>();
    file->file_path = upload_file_path;
    file->filename = req->filename;
    file->file_size = req->filesize;
    file->hash_count = 0;
    ctx.file = file;
    ctx.request = req.value();
    leaf::upload_file_response ufr;
    ufr.id = req->id;
    ufr.filename = req->filename;
    co_await channel_.async_send(ec, leaf::serialize_upload_file_response(ufr), boost::asio::redirect_error(boost::asio::use_awaitable, ec));
    co_return ctx;
}

boost::asio::awaitable<void> upload_file_handle::wait_ack(boost::beast::error_code& ec)
{
    boost::beast::flat_buffer buffer;
    co_await session_->read(ec, buffer);
    if (ec)
    {
        LOG_ERROR("{} recv_coro error {}", id_, ec.message());
        co_return;
    }
    auto message = boost::beast::buffers_to_string(buffer.data());
    buffer.consume(buffer.size());
    auto type = leaf::get_message_type(message);
    if (type != leaf::message_type::ack)
    {
        ec = boost::system::errc::make_error_code(boost::system::errc::protocol_error);
        co_return;
    }
}

boost::asio::awaitable<void> upload_file_handle::wait_file_data(leaf::upload_file_handle::upload_context& ctx, boost::beast::error_code& ec)
{
    auto hash = std::make_shared<leaf::blake2b>();
    auto writer = std::make_shared<leaf::file_writer>(ctx.file->file_path);
    ec = writer->open();
    if (ec)
    {
        LOG_ERROR("{} upload_file open file {} error {}", id_, ctx.file->file_path, ec.message());
        co_return;
    }

    while (true)
    {
        boost::beast::flat_buffer buffer;
        co_await session_->read(ec, buffer);
        if (ec)
        {
            LOG_ERROR("{} recv_coro error {}", id_, ec.message());
            break;
        }
        auto message = boost::beast::buffers_to_string(buffer.data());
        auto type = leaf::get_message_type(message);
        if (type == leaf::message_type::done)
        {
            LOG_INFO("{} upload file {} done", id_, ctx.file->filename);
            break;
        }
        if (type != leaf::message_type::file_data)
        {
            ec = boost::system::errc::make_error_code(boost::system::errc::protocol_error);
            break;
        }
        auto bytes = std::vector<uint8_t>(message.begin(), message.end());
        auto d = leaf::deserialize_file_data(bytes);
        if (!d.has_value())
        {
            break;
        }
        assert(d->data.size() <= kBlockSize);
        writer->write_at(static_cast<int64_t>(writer->size()), d->data.data(), d->data.size(), ec);
        if (ec)
        {
            LOG_ERROR("{} upload file write error {} {}", id_, ctx.file->filename, ec.message());
            break;
        }
        ctx.file->hash_count++;
        hash->update(d->data.data(), d->data.size());

        LOG_DEBUG("{} upload file {} hash count {} hash {} data size {} write size {}",
                  id_,
                  ctx.file->filename,
                  ctx.file->hash_count,
                  d->hash.empty() ? "empty" : d->hash,
                  d->data.size(),
                  writer->size());

        if (!d->hash.empty())
        {
            hash->final();
            auto hex_str = hash->hex();
            if (hex_str != d->hash)
            {
                LOG_ERROR("{} upload file hash not match {} {} {}", id_, ctx.file->filename, hex_str, d->hash);
                break;
            }
            ctx.file->hash_count = 0;
            hash = std::make_shared<leaf::blake2b>();
        }
        if (ctx.file->file_size == writer->size())
        {
            auto filename = leaf::encode_leaf_filename(leaf::make_file_path(token_, ctx.file->filename));
            leaf::rename(ctx.file->file_path, filename);
            LOG_INFO("{} upload file {} to {} done", id_, ctx.file->file_path, filename);
        }
    }
    ec = writer->close();
    if (ec)
    {
        LOG_ERROR("{} upload file close file {} error {}", id_, ctx.file->file_path, ec.message());
    }
}

boost::asio::awaitable<void> upload_file_handle::error_message(uint32_t id, int32_t error_code)
{
    boost::system::error_code ec;
    leaf::error_message e;
    e.id = id;
    e.error = error_code;
    auto bytes = leaf::serialize_error_message(e);
    co_await channel_.async_send(boost::system::error_code{}, bytes, boost::asio::redirect_error(boost::asio::use_awaitable, ec));
}

boost::asio::awaitable<void> upload_file_handle::on_keepalive(boost::beast::error_code& ec)
{
    boost::beast::flat_buffer buffer;
    co_await session_->read(ec, buffer);
    if (ec)
    {
        LOG_ERROR("{} recv_coro error {}", id_, ec.message());
        co_return;
    }
    auto message = boost::beast::buffers_to_string(buffer.data());
    auto type = leaf::get_message_type(message);
    if (type != leaf::message_type::keepalive)
    {
        ec = boost::system::errc::make_error_code(boost::system::errc::protocol_error);
        co_return;
    }
    auto k = leaf::deserialize_keepalive_response(std::vector<uint8_t>(message.begin(), message.end()));
    if (!k.has_value())
    {
        ec = boost::system::errc::make_error_code(boost::system::errc::protocol_error);
        co_return;
    }

    auto sk = k.value();

    sk.server_timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    LOG_DEBUG("{} on_keepalive client {} server_timestamp {} client_timestamp {} token {}",
              id_,
              sk.client_id,
              sk.server_timestamp,
              sk.client_timestamp,
              token_);
    co_await channel_.async_send(ec, serialize_keepalive(sk), boost::asio::redirect_error(boost::asio::use_awaitable, ec));
}
}    // namespace leaf
