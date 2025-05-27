#include <stack>
#include <string>
#include <filesystem>

#include "log/log.h"
#include "file/file.h"
#include "crypt/easy.h"
#include "config/config.h"
#include "protocol/codec.h"
#include "file/cotrol_file_handle.h"

namespace leaf
{
cotrol_file_handle ::cotrol_file_handle(const boost::asio::any_io_executor& io, std::string id, leaf::websocket_session::ptr& session)
    : id_(std::move(id)), session_(session), io_(io)
{
    LOG_INFO("create {}", id_);
}

cotrol_file_handle::~cotrol_file_handle() { LOG_INFO("destroy {}", id_); }

void cotrol_file_handle ::startup()
{
    LOG_INFO("startup {}", id_);

    boost::asio::co_spawn(io_, [this, self = shared_from_this()]() -> boost::asio::awaitable<void> { co_await recv_coro(); }, boost::asio::detached);

    boost::asio::co_spawn(io_, [this, self = shared_from_this()]() -> boost::asio::awaitable<void> { co_await write_coro(); }, boost::asio::detached);
}
boost::asio::awaitable<void> cotrol_file_handle ::write_coro()
{
    LOG_INFO("{} write coro startup", id_);
    while (true)
    {
        boost::system::error_code ec;
        auto bytes = co_await channel_.async_receive(boost::asio::redirect_error(boost::asio::use_awaitable, ec));
        if (ec)
        {
            LOG_ERROR("{} write_coro error {}", id_, ec.message());
            break;
        }
        co_await session_->write(ec, bytes.data(), bytes.size());
        if (ec)
        {
            LOG_ERROR("{} write_coro error {}", id_, ec.message());
            break;
        }
    }
    LOG_INFO("{} write coro shutdown", id_);
}

void cotrol_file_handle ::shutdown()
{
    boost::asio::co_spawn(
        io_, [this, self = shared_from_this()]() -> boost::asio::awaitable<void> { co_await shutdown_coro(); }, boost::asio::detached);
}

boost::asio::awaitable<void> cotrol_file_handle::shutdown_coro()
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
boost::asio::awaitable<void> cotrol_file_handle::recv_coro()
{
    LOG_INFO("{} recv coro startup", id_);
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
        boost::beast::flat_buffer buffer;
        co_await session_->read(ec, buffer);
        if (ec)
        {
            LOG_ERROR("{} recv error {}", id_, ec.message());
            co_return;
        }
        auto message = boost::beast::buffers_to_string(buffer.data());
        auto type = leaf::get_message_type(message);
        if (type == leaf::message_type::files_request)
        {
            co_await on_files_request(message, ec);
        }
        if (type == leaf::message_type::dir)
        {
            co_await on_create_dir(message, ec);
        }
        if (ec)
        {
            LOG_ERROR("{} process message error {}", id_, ec.message());
            break;
        }
    }
    LOG_INFO("{} recv coro shutdown", id_);
}

boost::asio::awaitable<void> cotrol_file_handle::keepalive(boost::beast::error_code& ec)
{
    boost::beast::flat_buffer buffer;
    co_await session_->read(ec, buffer);
    if (ec)
    {
        LOG_ERROR("{} keepalive error {}", id_, ec.message());
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
    LOG_DEBUG("{} keepalive client {} server_timestamp {} client_timestamp {} token {}",
              id_,
              sk.client_id,
              sk.server_timestamp,
              sk.client_timestamp,
              token_);
    co_await channel_.async_send(ec, serialize_keepalive(sk), boost::asio::redirect_error(boost::asio::use_awaitable, ec));
}
boost::asio::awaitable<void> cotrol_file_handle::wait_login(boost::beast::error_code& ec)
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
        LOG_ERROR("{} login error {}", id_, ec.message());
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
static std::vector<leaf::file_node> lookup_dir(const std::filesystem::path& dir)
{
    if (!std::filesystem::exists(dir))
    {
        return {};
    }
    std::stack<std::string> dirs;
    dirs.push(dir);
    std::vector<leaf::file_node> files;
    while (!dirs.empty())
    {
        std::string path = dirs.top();
        dirs.pop();
        for (const auto& entry : std::filesystem::directory_iterator(path))
        {
            std::string filename = entry.path().string();
            leaf::file_node f;
            f.name = filename;
            f.parent = path;
            f.type = "file";
            if (entry.is_directory())
            {
                f.type = "dir";
                dirs.push(entry.path().string());
            }
            else if (entry.is_regular_file())
            {
                std::string ext = entry.path().extension();
                if (ext != kLeafFilenameSuffix)
                {
                    continue;
                }
            }
            files.push_back(f);
        }
    }
    return files;
}

boost::asio::awaitable<void> cotrol_file_handle::on_files_request(const std::string& message, boost::beast::error_code& ec)
{
    auto files_request = leaf::deserialize_files_request(std::vector<uint8_t>(message.begin(), message.end()));
    if (!files_request.has_value())
    {
        ec = boost::system::errc::make_error_code(boost::system::errc::protocol_error);
        co_return;
    }

    const auto& msg = files_request.value();
    std::string user_path = leaf::make_file_path(msg.token);
    auto dir_path = leaf::make_file_path(msg.token, msg.dir);
    LOG_INFO("{} on files request dir {}", id_, dir_path);
    leaf::files_response response;
    // 递归遍历目录中的所有文件
    auto files = lookup_dir(dir_path);
    for (auto&& file : files)
    {
        std::string filename = std::filesystem::relative(file.name, user_path).stem().string();
        LOG_INFO("file name {} relative to user path {}", filename, user_path);
        file.name = leaf::decode(filename);
    }
    response.token = msg.token;
    response.files.swap(files);
    co_await channel_.async_send(ec, leaf::serialize_files_response(response), boost::asio::redirect_error(boost::asio::use_awaitable, ec));
}

boost::asio::awaitable<void> cotrol_file_handle::on_create_dir(const std::string& message, boost::beast::error_code& ec)
{
    auto dir_request = leaf::deserialize_create_dir(std::vector<uint8_t>(message.begin(), message.end()));
    if (!dir_request.has_value())
    {
        ec = boost::system::errc::make_error_code(boost::system::errc::protocol_error);
        co_return;
    }

    auto dir_path = leaf::make_file_path(dir_request->token, dir_request->dir);
    if (dir_path.empty())
    {
        LOG_ERROR("{} create dir {} failed", id_, dir_request->dir);
        ec = boost::system::errc::make_error_code(boost::system::errc::no_such_file_or_directory);
        co_return;
    }
    LOG_INFO("{} create dir {} --> {}", id_, dir_request->dir, dir_path);
}

}    // namespace leaf
