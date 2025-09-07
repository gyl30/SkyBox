#include <iostream>
#include <string>
#include <cstdint>
#include <filesystem>
#include <boost/asio.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/program_options.hpp>
#include "log/log.h"
#include "file/event.h"
#include "crypt/random.h"
#include "net/scoped_exit.hpp"
#include "file/event_manager.h"
#include "file/file_transfer_client.h"

static void download_progress(const std::any &data)
{
    auto e = std::any_cast<leaf::download_event>(data);
    LOG_INFO("--> download progress {} {} {}", e.filename, e.process_size, e.file_size);
}

static void upload_progress(const std::any &data)
{
    auto e = std::any_cast<leaf::file_event>(data);
    LOG_INFO("<-- upload progress {} {} {}", e.filename, e.process_size, e.file_size);
}

static void cotrol_progress(const std::any &data)
{
    //
    auto e = std::any_cast<leaf::cotrol_event>(data);
    LOG_INFO("^^^ cotrol progress {}", e.token);
}

static void notify_progress(const std::any &data)
{
    auto e = std::any_cast<leaf::notify_event>(data);
    if (e.method == "file_not_exist")
    {
        auto filename = std::any_cast<std::string>(e.data);
        LOG_INFO("download_file_not_exist {}", filename);
        return;
    }
    if (e.method == "files")
    {
        auto files = std::any_cast<std::vector<leaf::file_node>>(e.data);
        for (const auto &file : files)
        {
            LOG_INFO("--- {} {}", file.type, file.name);
        }
        return;
    }

    LOG_INFO("||| notify {}", e.method);
}
static void error_progress(const std::any &data)
{
    auto e = std::any_cast<leaf::error_event>(data);
    LOG_ERROR("!!! error {} {}", e.action, e.message);
}

struct command_args
{
    uint16_t port = 8080;
    std::string ip{"127.0.0.1"};
    std::string level{"info"};
    std::string username{"admin"};
    std::string password{"123456"};
    std::vector<std::string> upload_paths;
    std::vector<std::string> download_paths;
};

static std::string to_string(const command_args &args)
{
    return fmt::format("ip: {}, port: {}, username: {}, password: {}, upload_paths: {} -- download_paths: {}",
                       args.ip,
                       args.port,
                       args.username,
                       args.password,
                       fmt::join(args.upload_paths, ","),
                       fmt::join(args.download_paths, ","));
}

std::optional<command_args> parse_command_line(int argc, char *argv[])
{
    command_args args;
    boost::program_options::options_description desc("Allowed options");
    // clang-format off
    desc.add_options()("help,h", "Show help message")(
        "ip", boost::program_options::value<std::string>(&args.ip), "IP address (required)")(
        "port", boost::program_options::value<uint16_t>(&args.port), "Port number (required)")(
        "username", boost::program_options::value<std::string>(&args.username), "Username (required)")(
        "password", boost::program_options::value<std::string>(&args.password), "Password (required)")(
        "upload,u", boost::program_options::value<std::vector<std::string>>(&args.upload_paths)->multitoken(), "Upload files or folders (optional)")(
        "download,d", boost::program_options::value<std::vector<std::string>>(&args.download_paths)->multitoken(), "Download files or folders (optional)")(
        "log-level,l", boost::program_options::value<std::string>(&args.level)->default_value("info"), "Log level: info, debug, or error");
    // clang-format on

    auto parsed = boost::program_options::command_line_parser(argc, argv).options(desc).allow_unregistered().run();
    boost::program_options::variables_map vm;
    boost::program_options::store(parsed, vm);
    boost::program_options::notify(vm);
    if (vm.contains("help"))
    {
        std::cerr << desc << "\n";
        return {};
    }

    return args;
}
static boost::asio::awaitable<std::string> login(const command_args &args, boost::system::error_code &ec)
{
    leaf::login_request login_request;
    login_request.username = args.username;
    login_request.password = args.password;
    auto data = leaf::serialize_login_request(login_request);
    std::string target = "/leaf/login";
    boost::beast::http::request<boost::beast::http::string_body> req;
    req.version(11);
    req.method(boost::beast::http::verb::post);
    req.target(target);
    req.set(boost::beast::http::field::host, args.ip);
    req.set(boost::beast::http::field::content_type, "application/json");
    req.set(boost::beast::http::field::user_agent, "leaf/http");
    req.body() = std::string(data.begin(), data.end());
    req.prepare_payload();
    auto resolver = boost::asio::use_awaitable_t<boost::asio::any_io_executor>::as_default_on(
        boost::asio::ip::tcp::resolver(co_await boost::asio::this_coro::executor));

    auto const results = co_await resolver.async_resolve(args.ip, std::to_string(args.port));

    leaf::tcp_stream_limited stream(co_await boost::asio::this_coro::executor);
    stream.expires_after(std::chrono::seconds(30));
    std::string id_ = leaf::random_string(8);
    auto ep = co_await stream.async_connect(results, boost::asio::redirect_error(boost::asio::use_awaitable, ec));
    if (ec)
    {
        LOG_ERROR("{} connect {}:{} failed {}", id_, args.ip, args.port, ec.message());
        co_return "";
    }
    std::string host = args.ip + ':' + std::to_string(ep.port());
    LOG_INFO("{} connect {} success", id_, host);
    stream.expires_after(std::chrono::seconds(30));
    co_await boost::beast::http::async_write(stream, req, boost::asio::redirect_error(boost::asio::use_awaitable, ec));
    if (ec)
    {
        LOG_ERROR("{} write to {} failed {}", id_, host, ec.message());
        co_return "";
    }
    boost::beast::flat_buffer buffer;
    boost::beast::http::response<boost::beast::http::vector_body<uint8_t>> res;
    stream.expires_after(std::chrono::seconds(5));
    co_await boost::beast::http::async_read(stream, buffer, res, boost::asio::redirect_error(boost::asio::use_awaitable, ec));
    if (ec)
    {
        LOG_ERROR("{} read from {} failed {}", id_, host, ec.message());
        co_return "";
    }
    auto login_response = leaf::deserialize_login_token(res.body());
    if (!login_response)
    {
        LOG_ERROR("{} login deserialize failed", id_);
        co_return "";
    }
    stream.socket().close();
    co_return login_response->token;
}

int main(int argc, char *argv[])
{
    auto args = parse_command_line(argc, argv);
    if (!args)
    {
        return -1;
    }

    leaf::init_log("cli.log");
    DEFER(leaf::shutdown_log());

    LOG_INFO("command args {}", to_string(args.value()));

    leaf::set_log_level(args->level);

    leaf::event_manager::instance().startup();

    DEFER(leaf::event_manager::instance().shutdown());

    boost::system::error_code ec;
    boost::asio::io_context io;
    std::string token;
    boost::asio::co_spawn(io, [&]() -> boost::asio::awaitable<void> { token = co_await login(args.value(), ec); }, boost::asio::detached);
    io.run(ec);
    if (token.empty())
    {
        LOG_ERROR("login failed {}", ec.message());
        return -1;
    }

    leaf::event_manager::instance().subscribe("error", [](const std::any &data) { error_progress(data); });
    leaf::event_manager::instance().subscribe("notify", [](const std::any &data) { notify_progress(data); });
    leaf::event_manager::instance().subscribe("upload", [](const std::any &data) { upload_progress(data); });
    leaf::event_manager::instance().subscribe("cotrol", [](const std::any &data) { cotrol_progress(data); });
    leaf::event_manager::instance().subscribe("download", [](const std::any &data) { download_progress(data); });

    auto fm = std::make_shared<leaf::file_transfer_client>(args->ip, args->port, args->username, args->password, token);

    fm->startup();

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    std::vector<leaf::file_info> download_files;
    for (const auto &dir : args->download_paths)
    {
        leaf::file_info f;
        f.filename = dir;
        download_files.push_back(f);
    }

    std::vector<leaf::file_info> upload_files;
    for (const auto &path : args->upload_paths)
    {
        leaf::file_info f;
        if (leaf::is_file(path))
        {
            f.filename = std::filesystem::path(path).filename().string();
            f.local_path = path;
            f.file_size = std::filesystem::file_size(path);
            f.dir = ".";
            upload_files.push_back(f);
        }
        if (leaf::is_dir(path))
        {
            auto files = leaf::dir_files(path);
            for (const auto &file : files)
            {
                f.filename = std::filesystem::path(file).filename().string();
                f.local_path = file;
                f.file_size = std::filesystem::file_size(file);
                f.dir = ".";
                upload_files.push_back(f);
            }
        }
    }

    fm->add_upload_files(upload_files);
    fm->add_download_files(download_files);

    std::this_thread::sleep_for(std::chrono::seconds(60));

    fm->shutdown();
    return 0;
}
