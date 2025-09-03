#include <utility>
#include <filesystem>
#include "log/log.h"
#include "file/file.h"
#include "crypt/easy.h"
#include "crypt/passwd.h"
#include "net/exception.h"
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

    auto msg = fmt::format("upload file handle startup {}", token_);
    boost::asio::co_spawn(
        io_,
        [this, self = shared_from_this()]() -> boost::asio::awaitable<void> { co_await loop(); },
        [msg](const std::exception_ptr& e) { leaf::cache_exception(msg, e); });
}

boost::asio::awaitable<void> upload_file_handle::write(const std::vector<uint8_t>& msg, boost::beast::error_code& ec)
{
    co_await session_->write(ec, msg.data(), msg.size());
}

void upload_file_handle ::shutdown()
{
    auto msg = fmt::format("upload file handle shutdown {}", token_);
    boost::asio::co_spawn(
        io_,
        [this, self = shared_from_this()]() -> boost::asio::awaitable<void> { co_await shutdown_coro(); },
        [msg](const std::exception_ptr& e) { leaf::cache_exception(msg, e); });
}

boost::asio::awaitable<void> upload_file_handle::shutdown_coro()
{
    if (session_)
    {
        session_->close();
        session_.reset();
    }
    LOG_INFO("{} shutdown", id_);
    co_return;
}
boost::asio::awaitable<void> upload_file_handle::loop()
{
    LOG_INFO("{} recv startup", id_);
    boost::beast::error_code ec;
    co_await session_->handshake(ec);
    if (ec)
    {
        LOG_ERROR("{} handshake error {}", id_, ec.message());
        co_return;
    }
    LOG_INFO("{} handshake success", id_);
    session_->set_read_limit(kMB * 10);
    session_->set_write_limit(kKB);
    // setup 1 wait login
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
        LOG_INFO("{} wait keepalive", id_);
        auto k = co_await wait_keepalive(ec);
        if (ec)
        {
            LOG_ERROR("{} wait keepalive error {}", id_, ec.message());
            break;
        }
        LOG_DEBUG("{} wait keepalive server timestamp {}", id_, k.server_timestamp);
        if (k.server_timestamp == 0)
        {
            continue;    // ignore keepalive with server_timestamp 0
        }
        LOG_INFO("{} wait upload file request", id_);
        // setup 2 wait upload file request
        auto file = co_await wait_upload_file_request(ec);
        if (ec)
        {
            LOG_ERROR("{} upload file request error {}", id_, ec.message());
            break;
        }
        LOG_INFO("{} upload file request success filename {} filesize {}", id_, file.filename, file.file_size);
        // setup 3 wait ack
        LOG_INFO("{} wait ack {}", id_, file.filename);
        co_await wait_ack(ec);
        if (ec)
        {
            LOG_ERROR("{} ack error {} {}", id_, ec.message(), file.filename);
            break;
        }
        LOG_INFO("{} ack success {}", id_, file.filename);
        // setup 4 wait file data
        LOG_INFO("{} wait file data {}", id_, file.filename);
        co_await wait_file_data(file, ec);
        if (ec)
        {
            LOG_ERROR("{} file data error {} {}", id_, ec.message(), file.filename);
            break;
        }
        LOG_INFO("{} send file done {}", id_, file.local_path);
        co_await send_file_done(ec);
        if (ec)
        {
            LOG_ERROR("{} send file done error {} {}", id_, file.local_path, ec.message());
        }
    }
    LOG_INFO("{} recv shutdown", id_);
}

boost::asio::awaitable<void> upload_file_handle::wait_login(boost::beast::error_code& ec)
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

boost::asio::awaitable<leaf::file_info> upload_file_handle::wait_upload_file_request(boost::beast::error_code& ec)
{
    leaf::file_info file;
    boost::beast::flat_buffer buffer;
    co_await session_->read(ec, buffer);
    if (ec)
    {
        co_return file;
    }
    auto message = boost::beast::buffers_to_string(buffer.data());
    buffer.consume(buffer.size());
    auto type = leaf::get_message_type(message);
    if (type != leaf::message_type::upload_file_request)
    {
        ec = boost::system::errc::make_error_code(boost::system::errc::protocol_error);
        co_return file;
    }
    auto bytes = std::vector<uint8_t>(message.begin(), message.end());
    auto req = leaf::deserialize_upload_file_request(bytes);
    if (!req.has_value())
    {
        ec = boost::system::errc::make_error_code(boost::system::errc::protocol_error);
        co_return file;
    }
    auto local_path = std::filesystem::path(token_).append(req->dir).string();
    auto file_path = leaf::make_file_path(local_path, leaf::encode(req->filename));
    auto upload_file_path = leaf::encode_tmp_filename(file_path);
    bool exist = std::filesystem::exists(upload_file_path, ec);
    if (ec)
    {
        LOG_ERROR("{} upload request file {} exist failed {}", id_, upload_file_path, ec.message());
        co_return file;
    }
    if (exist)
    {
        ec = {};
        LOG_ERROR("{} upload request file {} exist", id_, upload_file_path);
        std::filesystem::remove(upload_file_path, ec);
        if (ec)
        {
            ec = boost::system::errc::make_error_code(boost::system::errc::file_exists);
            co_return file;
        }
        LOG_WARN("{} upload request file {} remove success", id_, upload_file_path);
    }
    LOG_INFO("{} upload request file size {} name {} path {} dir {} local path {}",
             id_,
             req->filesize,
             req->filename,
             req->dir,
             upload_file_path,
             local_path);

    file.filename = req->filename;
    file.file_size = req->filesize;
    file.dir = local_path;
    file.local_path = upload_file_path;
    leaf::upload_file_response ufr;
    ufr.id = req->id;
    ufr.filename = req->filename;
    co_await write(leaf::serialize_upload_file_response(ufr), ec);
    co_return file;
}

boost::asio::awaitable<void> upload_file_handle::wait_ack(boost::beast::error_code& ec)
{
    boost::beast::flat_buffer buffer;
    co_await session_->read(ec, buffer);
    if (ec)
    {
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

boost::asio::awaitable<void> upload_file_handle::send_file_done(boost::beast::error_code& ec)
{
    leaf::done d;
    co_await write(leaf::serialize_done(d), ec);
}

boost::asio::awaitable<void> upload_file_handle::wait_file_data(leaf::file_info& file, boost::beast::error_code& ec)
{
    auto hash = std::make_shared<leaf::blake2b>();
    auto writer = std::make_shared<leaf::file_writer>(file.local_path);
    ec = writer->open();
    if (ec)
    {
        LOG_ERROR("{} open file {} error {}", id_, file.local_path, ec.message());
        co_return;
    }
    bool done = false;
    auto filename = leaf::encode_leaf_filename(file.local_path);
    while (true)
    {
        boost::beast::flat_buffer buffer;
        co_await session_->read(ec, buffer);
        if (ec)
        {
            break;
        }
        auto message = boost::beast::buffers_to_string(buffer.data());
        auto type = leaf::get_message_type(message);
        if (type == leaf::message_type::done)
        {
            done = true;
            leaf::rename(file.local_path, filename);

            LOG_INFO("{} upload file {} done", id_, file.filename);
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
            LOG_ERROR("{} file write error {} {}", id_, file.filename, ec.message());
            break;
        }
        hash->update(d->data.data(), d->data.size());

        LOG_DEBUG(
            "{} file {} hash {} data size {} write size {}", id_, file.filename, d->hash.empty() ? "empty" : d->hash, d->data.size(), writer->size());

        if (!d->hash.empty())
        {
            hash->final();
            auto hex_str = hash->hex();
            if (hex_str != d->hash)
            {
                LOG_ERROR("{} file hash not match {} {} {}", id_, file.filename, hex_str, d->hash);
                break;
            }
            hash->reset();
        }
        if (file.file_size == writer->size())
        {
            LOG_INFO("{} file size done {} to {}", id_, file.filename, filename);
        }
    }
    auto file_ec = writer->close();
    if (file_ec)
    {
        LOG_ERROR("{} file close file {} error {}", id_, file.local_path, file_ec.message());
    }
    if (!done)
    {
        co_return;
    }
    LOG_INFO("{} upload file {} done", id_, file.filename);
}

boost::asio::awaitable<leaf::keepalive> upload_file_handle::wait_keepalive(boost::beast::error_code& ec)
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
    LOG_DEBUG(
        "{} keepalive client {} server_timestamp {} client_timestamp {} token {}", id_, k.client_id, k.server_timestamp, k.client_timestamp, token_);
    co_await write(serialize_keepalive(k), ec);
    co_return keepalive_request.value();
}
}    // namespace leaf
