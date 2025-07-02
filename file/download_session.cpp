#include <utility>
#include <string>
#include <filesystem>
#include "log/log.h"
#include "file/file.h"
#include "protocol/codec.h"
#include "file/event_manager.h"
#include "file/download_session.h"

namespace leaf
{
download_session::download_session(std::string id, std::string host, std::string port, std::string token, boost::asio::io_context& io)
    : id_(std::move(id)), host_(std::move(host)), port_(std::move(port)), token_(std::move(token)), io_(io)
{
    LOG_INFO("{} startup", id_);
}

download_session::~download_session() { LOG_INFO("{} shutdown", id_); }

void download_session::startup()
{
    LOG_INFO("{} startup", id_);

    ws_client_ = std::make_shared<leaf::plain_websocket_client>(id_, host_, port_, "/leaf/ws/download", io_);

    boost::asio::co_spawn(io_, [this, self = shared_from_this()]() -> boost::asio::awaitable<void> { co_await loop(); }, boost::asio::detached);
}

boost::asio::awaitable<void> download_session::send_keepalive(boost::beast::error_code& ec)
{
    leaf::keepalive k;
    k.id = 0;
    k.client_id = reinterpret_cast<uintptr_t>(this);
    k.client_timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    k.server_timestamp = padding_files_.size();
    co_await write(leaf::serialize_keepalive(k), ec);
    if (ec)
    {
        LOG_ERROR("{} send keepalive error {}", id_, ec.message());
        co_return;
    }
    boost::beast::flat_buffer buffer;
    co_await ws_client_->read(ec, buffer);
    if (ec)
    {
        LOG_ERROR("{} read keepalive error {}", id_, ec.message());
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
    LOG_DEBUG("{} on_keepalive client {} server_timestamp {} client_timestamp {} token {}",
              id_,
              kk->client_id,
              kk->server_timestamp,
              kk->client_timestamp,
              token_);
}

boost::asio::awaitable<void> download_session::login(boost::beast::error_code& ec)
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
    LOG_INFO("{} login success token {}", id_, login->token);
}
boost::asio::awaitable<void> download_session::loop()
{
    LOG_INFO("{} download coro startup", id_);
    boost::beast::error_code ec;
    co_await ws_client_->handshake(ec);
    if (ec)
    {
        LOG_ERROR("{} download coro handshake error {}", id_, ec.message());
        co_return;
    }
    co_await login(ec);
    if (ec)
    {
        LOG_ERROR("{} download coro login error {}", id_, ec.message());
        co_return;
    }
    while (true)
    {
        LOG_INFO("{} download file size {}", id_, padding_files_.size());
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
        co_await download(ec);
        if (ec)
        {
            LOG_ERROR("{} download error {}", id_, ec.message());
            break;
        }
    }

    LOG_INFO("{} download coro shutdown", id_);
}
boost::asio::awaitable<void> download_session::delay(int second)
{
    boost::beast::error_code ec;
    boost::asio::steady_timer timer(io_, std::chrono::seconds(second));
    co_await timer.async_wait(boost::asio::redirect_error(boost::asio::use_awaitable, ec));
}

boost::asio::awaitable<void> download_session::write(const std::vector<uint8_t>& data, boost::system::error_code& ec)
{
    co_await ws_client_->write(ec, data.data(), data.size());
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
        ws_client_->close();
        ws_client_.reset();
    }
    LOG_INFO("{} shutdown", id_);
    co_return;
}

boost::asio::awaitable<void> download_session::download(boost::beast::error_code& ec)
{
    while (!padding_files_.empty())
    {
        auto file = padding_files_.front();
        padding_files_.pop();
        // send download file request
        LOG_INFO("{} send download file request {}", id_, file.local_path);
        co_await send_download_file_request(file, ec);
        if (ec)
        {
            LOG_ERROR("{} send download file request error {} {}", id_, file.local_path, ec.message());
            break;
        }
        LOG_INFO("{} send download file request success {}", id_, file.local_path);
        // wait download file response
        auto ctx = co_await wait_download_file_response(ec);
        if (ec)
        {
            LOG_ERROR("{} wait download file response error {} {}", id_, file.local_path, ec.message());
            break;
        }
        LOG_INFO("{} wait download file response success {} file size {}", id_, ctx.response.filename, ctx.response.filesize);
        // wait ack message
        LOG_INFO("{} wait ack for download file {}", id_, ctx.response.filename);
        co_await wait_ack(ec);
        if (ec)
        {
            LOG_ERROR("{} wait ack error {} {}", id_, file.local_path, ec.message());
            break;
        }
        LOG_INFO("{} wait ack success for download file {}", id_, ctx.response.filename);
        // wait file data
        LOG_INFO("{} wait file data for download file {}", id_, ctx.response.filename);
        co_await wait_file_data(ctx, ec);
        if (ec)
        {
            LOG_ERROR("{} wait file data error {} {}", id_, ctx.response.filename, ec.message());
            break;
        }
        LOG_INFO("{} wait file data success for download file {}", id_, ctx.response.filename);
        // wait file done
        LOG_INFO("{} wait file done for download file {}", id_, ctx.response.filename);
        co_await wait_file_done(ec);
        if (ec)
        {
            LOG_ERROR("{} wait file done error {} {}", id_, ctx.response.filename, ec.message());
            break;
        }
        LOG_INFO("{} wait file done success for download file {}", id_, ctx.response.filename);
    }
    co_return;
}

boost::asio::awaitable<void> download_session::send_download_file_request(const file_info& file, boost::beast::error_code& ec)
{
    leaf::download_file_request req;
    req.dir = file.dir;
    req.filename = file.filename;
    req.id = ++seq_;
    co_await write(leaf::serialize_download_file_request(req), ec);
}

boost::asio::awaitable<leaf::download_session::download_context> download_session::wait_download_file_response(boost::beast::error_code& ec)
{
    leaf::download_session::download_context ctx;
    boost::beast::flat_buffer buffer;
    co_await ws_client_->read(ec, buffer);
    if (ec)
    {
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
    const leaf::download_file_response& response = download_response.value();
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
    file->local_path = file_path;
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
    auto writer = std::make_shared<leaf::file_writer>(ctx.file->local_path);
    ec = writer->open();
    if (ec)
    {
        LOG_ERROR("{} wait file data writer open error {}", id_, ec.message());
        co_return;
    }

    auto hash = std::make_shared<leaf::blake2b>();

    int64_t write_offset = 0;
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

        writer->write_at(write_offset, data->data.data(), data->data.size(), ec);
        if (ec)
        {
            LOG_ERROR("{} wait file data writer write error {}", id_, ec.message());
            break;
        }
        write_offset += static_cast<int64_t>(data->data.size());
        hash->update(data->data.data(), data->data.size());
        LOG_DEBUG("{} download file {} hash {} data size {} write size {}",
                  id_,
                  ctx.file->local_path,
                  data->hash.empty() ? "empty" : data->hash,
                  data->data.size(),
                  writer->size());

        if (!data->hash.empty())
        {
            hash->final();
            auto hex_str = hash->hex();
            if (hex_str != data->hash)
            {
                LOG_ERROR("{} download file {} hash not match {} {}", id_, ctx.file->local_path, hex_str, data->hash);
                break;
            }
            hash.reset();
        }
        download_event d;
        d.filename = ctx.file->filename;
        d.download_size = writer->size();
        d.file_size = ctx.file->file_size;
        leaf::event_manager::instance().post("download", d);
        if (ctx.file->file_size == writer->size())
        {
            LOG_INFO("{} download file {} size {} done", id_, ctx.file->local_path, d.file_size);
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

void download_session::safe_add_file(leaf::file_info f) { padding_files_.emplace(std::move(f)); }

void download_session::safe_add_files(const std::vector<leaf::file_info>& files)
{
    for (const auto& filename : files)
    {
        padding_files_.push(filename);
    }
}

void download_session::add_file(leaf::file_info f)
{
    boost::asio::post(io_, [this, f = std::move(f), self = shared_from_this()]() { safe_add_file(f); });
}

void download_session::add_files(const std::vector<leaf::file_info>& files)
{
    boost::asio::post(io_, [this, files, self = shared_from_this()]() { safe_add_files(files); });
}

}    // namespace leaf
