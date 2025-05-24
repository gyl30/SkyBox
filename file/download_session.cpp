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
download_session::download_session(
    std::string id, std::string host, std::string port, std::string token, leaf::download_handle handler, boost::asio::io_context& io)
    : id_(std::move(id)), host_(std::move(host)), port_(std::move(port)), token_(std::move(token)), io_(io), progress_cb_(std::move(handler))
{
    LOG_INFO("{} startup", id_);
}

download_session::~download_session() { LOG_INFO("{} shutdown", id_); }

void download_session::startup()
{
    LOG_INFO("{} startup", id_);

    ws_client_ = std::make_shared<leaf::plain_websocket_client>(id_, host_, port_, "/leaf/ws/download", io_);

    boost::asio::co_spawn(
        io_, [this, self = shared_from_this()]() -> boost::asio::awaitable<void> { co_await download_coro(); }, boost::asio::detached);
    boost::asio::co_spawn(io_, [this, self = shared_from_this()]() -> boost::asio::awaitable<void> { co_await write_coro(); }, boost::asio::detached);
}

boost::asio::awaitable<void> download_session::keepalive(boost::beast::error_code& ec)
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
boost::asio::awaitable<void> download_session::download_coro()
{
    LOG_INFO("{} download coro startup", id_);
    boost::beast::error_code ec;
    co_await ws_client_->handshake(ec);
    if (ec)
    {
        LOG_ERROR("{} download coro handshake error {}", id_, ec.message());
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
        LOG_ERROR("{} download coro send login token error {}", id_, ec.message());
        co_return;
    }
    while (true)
    {
        // padding files empty will send keepalive message
        if (padding_files_.empty())
        {
            co_await keepalive(ec);
        }
        if (ec)
        {
            LOG_ERROR("{} download keepalive error {}", id_, ec.message());
            break;
        }
        co_await download(ec);
        if (ec)
        {
            LOG_ERROR("{} download error {}", id_, ec.message());
            break;
        }
    }

    LOG_INFO("{} download coro shutdown", id_);
}

boost::asio::awaitable<void> download_session::write_coro()
{
    boost::system::error_code ec;
    auto bytes = co_await channel_.async_receive(boost::asio::redirect_error(boost::asio::use_awaitable, ec));
    if (ec)
    {
        LOG_ERROR("{} write_coro error {}", id_, ec.message());
        co_return;
    }
    co_await ws_client_->write(ec, bytes.data(), bytes.size());
    if (ec)
    {
        LOG_ERROR("{} write_coro error {}", id_, ec.message());
    }
}

void download_session::shutdown()
{
    boost::asio::co_spawn(
        io_, [this, self = shared_from_this()]() -> boost::asio::awaitable<void> { co_await shutdown_coro(); }, boost::asio::detached);
}

boost::asio::awaitable<void> download_session::shutdown_coro()
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

boost::asio::awaitable<void> download_session::download(boost::beast::error_code& ec)
{
    while (true)
    {
        co_await keepalive(ec);
        if (ec)
        {
            LOG_ERROR("{} download keepalive error {}", id_, ec.message());
            break;
        }
        if (!padding_files_.empty())
        {
            break;
        }
        auto file = padding_files_.front();
        padding_files_.pop();
        // send download file request
        co_await send_download_file_request(file, ec);
        if (ec)
        {
            LOG_ERROR("{} send download file request error {}", id_, ec.message());
            break;
        }
        // wait download file response
        auto ctx = co_await wait_download_file_response(ec);
        if (ec)
        {
            LOG_ERROR("{} wait download file response error {}", id_, ec.message());
            break;
        }
        // wait ack message
        co_await wait_ack(ec);
        if (ec)
        {
            LOG_ERROR("{} wait ack error {}", id_, ec.message());
            break;
        }
        // wait file data
        co_await wait_file_data(ctx, ec);
        if (ec)
        {
            LOG_ERROR("{} wait file data error {}", id_, ec.message());
            break;
        }
        // wait file done
        co_await wait_file_done(ec);
        if (ec)
        {
            LOG_ERROR("{} wait file done error {}", id_, ec.message());
            break;
        }
    }
    co_return;
}

boost::asio::awaitable<void> download_session::send_download_file_request(const std::string& filename, boost::beast::error_code& ec)
{
    leaf::download_file_request req;
    req.filename = filename;
    req.id = ++seq_;
    LOG_INFO("{} download_file {}", id_, req.filename);
    co_await channel_.async_send(ec, leaf::serialize_download_file_request(req), boost::asio::redirect_error(boost::asio::use_awaitable, ec));
}

boost::asio::awaitable<leaf::download_session::download_context> download_session::wait_download_file_response(boost::beast::error_code& ec)
{
    leaf::download_session::download_context ctx;
    boost::beast::flat_buffer buffer;
    co_await ws_client_->read(ec, buffer);
    if (ec)
    {
        LOG_ERROR("{} wait download file response error {}", id_, ec.message());
        co_return ctx;
    }
    auto message = boost::beast::buffers_to_string(buffer.data());
    auto type = leaf::get_message_type(message);
    if (type != leaf::message_type::download_file_response)
    {
        ec = boost::system::errc::make_error_code(boost::system::errc::protocol_error);
        co_return ctx;
    }
    auto download_response = leaf::deserialize_download_file_response(std::vector<uint8_t>(message.begin(), message.end()));
    if (!download_response.has_value())
    {
        ec = boost::system::errc::make_error_code(boost::system::errc::protocol_error);
        co_return ctx;
    }
    leaf::download_file_response response = download_response.value();
    auto file_path = std::filesystem::path(response.filename).string();
    bool exists = std::filesystem::exists(file_path, ec);
    if (ec)
    {
        co_return ctx;
    }
    if (exists)
    {
        uint32_t exists_size = std::filesystem::file_size(file_path, ec);
        if (ec)
        {
            co_return ctx;
        }
        if (exists_size != response.filesize)
        {
            ec = boost::system::errc::make_error_code(boost::system::errc::no_such_process);
            co_return ctx;
        }
    }

    auto file = std::make_shared<leaf::file_info>();
    file->filename = response.filename;
    file->file_size = response.filesize;
    file->file_path = file_path;
    ctx.response = response;
    ctx.file = file;
    co_return ctx;
}

boost::asio::awaitable<void> download_session::wait_ack(boost::beast::error_code& ec)
{
    boost::beast::flat_buffer buffer;
    leaf::download_file_response response;
    co_await ws_client_->read(ec, buffer);
    if (ec)
    {
        LOG_ERROR("{} wait ack error {}", id_, ec.message());
        co_return;
    }
    auto message = boost::beast::buffers_to_string(buffer.data());
    auto type = leaf::get_message_type(message);
    if (type != leaf::message_type::ack)
    {
        ec = boost::system::errc::make_error_code(boost::system::errc::protocol_error);
        co_return;
    }
    auto ack = leaf::deserialize_ack(std::vector<uint8_t>(message.begin(), message.end()));
    if (!ack.has_value())
    {
        ec = boost::system::errc::make_error_code(boost::system::errc::protocol_error);
        co_return;
    }
}

boost::asio::awaitable<void> download_session::wait_file_data(leaf::download_session::download_context& ctx, boost::beast::error_code& ec)
{
    auto writer = std::make_shared<leaf::file_writer>(ctx.file->file_path);
    ec = writer->open();
    if (ec)
    {
        LOG_ERROR("{} wait file data writer open error {}", id_, ec.message());
        co_return;
    }

    auto hash = std::make_shared<leaf::blake2b>();

    while (true)
    {
        boost::beast::flat_buffer buffer;
        leaf::download_file_response response;
        co_await ws_client_->read(ec, buffer);
        if (ec)
        {
            LOG_ERROR("{} wait ack error {}", id_, ec.message());
            break;
        }
        auto message = boost::beast::buffers_to_string(buffer.data());
        auto type = leaf::get_message_type(message);
        if (type != leaf::message_type::file_data)
        {
            ec = boost::system::errc::make_error_code(boost::system::errc::protocol_error);
            break;
        }
        auto data = leaf::deserialize_file_data(std::vector<uint8_t>(message.begin(), message.end()));
        if (!data.has_value())
        {
            ec = boost::system::errc::make_error_code(boost::system::errc::protocol_error);
            break;
        }

        writer->write_at(ctx.file->offset, data->data.data(), data->data.size(), ec);
        if (ec)
        {
            LOG_ERROR("{} wait file data writer write error {}", id_, ec.message());
            break;
        }
        ctx.file->offset += static_cast<int64_t>(data->data.size());
        ctx.file->hash_count++;
        hash->update(data->data.data(), data->data.size());
        LOG_DEBUG("{} download file {} hash count {} hash {} data size {} write size {}",
                  id_,
                  ctx.file->file_path,
                  ctx.file->hash_count,
                  data->hash.empty() ? "empty" : data->hash,
                  data->data.size(),
                  writer->size());

        if (!data->hash.empty())
        {
            hash->final();
            auto hex_str = hash->hex();
            if (hex_str != data->hash)
            {
                LOG_ERROR("{} download file {} hash not match {} {}", id_, ctx.file->file_path, hex_str, data->hash);
                break;
            }
            ctx.file->hash_count = 0;
            hash = std::make_shared<leaf::blake2b>();
        }
        download_event d;
        d.filename = ctx.file->filename;
        d.download_size = writer->size();
        d.file_size = ctx.file->file_size;
        emit_event(d);
        if (ctx.file->file_size == writer->size())
        {
            LOG_INFO("{} download file {} size {} done", id_, ctx.file->file_path, d.file_size);
        }
    }
    ec = writer->close();
    if (ec)
    {
        LOG_ERROR("{} wait file data writer close error {}", id_, ec.message());
    }
}

boost::asio::awaitable<void> download_session::wait_file_done(boost::beast::error_code& ec)
{
    boost::beast::flat_buffer buffer;
    leaf::download_file_response response;
    co_await ws_client_->read(ec, buffer);
    if (ec)
    {
        LOG_ERROR("{} wait ack error {}", id_, ec.message());
        co_return;
    }
    auto message = boost::beast::buffers_to_string(buffer.data());
    auto type = leaf::get_message_type(message);
    if (type != leaf::message_type::done)
    {
        ec = boost::system::errc::make_error_code(boost::system::errc::protocol_error);
        co_return;
    }
    auto data = leaf::deserialize_done(std::vector<uint8_t>(message.begin(), message.end()));
    if (!data.has_value())
    {
        ec = boost::system::errc::make_error_code(boost::system::errc::protocol_error);
    }
}
void download_session::emit_event(const leaf::download_event& e) const
{
    if (progress_cb_.download)
    {
        progress_cb_.download(e);
    }
}

void download_session::safe_add_file(const std::string& filename) { padding_files_.push(filename); }

void download_session::safe_add_files(const std::vector<std::string>& files)
{
    for (const auto& filename : files)
    {
        padding_files_.push(filename);
    }
}

void download_session::add_file(const std::string& file)
{
    io_.post([this, file, self = shared_from_this()]() { safe_add_file(file); });
}

void download_session::add_files(const std::vector<std::string>& files)
{
    io_.post([this, files, self = shared_from_this()]() { safe_add_files(files); });
}

}    // namespace leaf
