#include <iostream>
#include <boost/program_options.hpp>
#include "log/log.h"
#include "file/event.h"
#include "net/scoped_exit.hpp"
#include "file/file_transfer_client.h"

static void download_progress(const leaf::download_event &e)
{
    LOG_INFO("--> download progress {} {} {}", e.filename, e.download_size, e.file_size);
}

static void upload_progress(const leaf::upload_event &e)
{
    LOG_INFO("<-- upload progress {} {} {}", e.filename, e.upload_size, e.file_size);
}
static void notify_progress(const leaf::notify_event &e)
{
    if (e.method == "file_not_exist")
    {
        auto filename = std::any_cast<std::string>(e.data);
        LOG_INFO("download_file_not_exist {}", filename);
        return;
    }
    LOG_INFO("||| notify {}", e.method);
}
static void error_progress(const boost::system::error_code &ec) { LOG_ERROR("^^^ error {}", ec.message()); }

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
    if (vm.count("help") != 0U)
    {
        std::cerr << desc << "\n";
        return {};
    }

    if (vm.count("ip") != 0U || vm.count("port") != 0U)
    {
        std::cerr << desc << "\n";
        return {};
    }
    if (vm.count("username") != 0U || vm.count("password") != 0U)
    {
        std::cerr << desc << "\n";
        return {};
    }

    return args;
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

    leaf::set_log_level(args->level);
    leaf::progress_handler handler;
    handler.d.download = download_progress;
    handler.d.notify = notify_progress;
    handler.u.upload = upload_progress;
    handler.u.notify = notify_progress;

    leaf::file_transfer_client fm(args->ip, args->port, handler);
    auto error = [&fm](const boost::system::error_code &ec)
    {
        error_progress(ec);
        fm.shutdown();
    };
    handler.d.error = error;
    handler.u.error = error;

    fm.startup();

    fm.login(args->username, args->password);

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    std::vector<std::string> download_files;
    for (const auto &dir : args->download_paths)
    {
        if (leaf::is_file(dir))
        {
            download_files.push_back(dir);
        }
        if (leaf::is_dir(dir))
        {
            auto files = leaf::dir_files(dir);
            download_files.insert(download_files.end(), files.begin(), files.end());
        }
    }

    std::vector<std::string> upload_files;
    for (const auto &dir : args->upload_paths)
    {
        if (leaf::is_file(dir))
        {
            upload_files.push_back(dir);
        }
        if (leaf::is_dir(dir))
        {
            auto files = leaf::dir_files(dir);
            upload_files.insert(upload_files.end(), files.begin(), files.end());
        }
    }

    fm.add_upload_files(download_files);
    fm.add_download_files(upload_files);

    std::this_thread::sleep_for(std::chrono::seconds(60));

    fm.shutdown();
    return 0;
}
