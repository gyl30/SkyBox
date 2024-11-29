#include <iostream>

#include "log.h"
#include "event.h"
#include "file_context.h"
#include "upload_session.h"
#include "download_session.h"
#include "plain_websocket_client.h"

static void download_progress(const leaf::download_event& e)
{
    LOG_INFO("download progress {} {} {} {}", e.id, e.filename, e.download_size, e.file_size);
}

static void upload_progress(const leaf::upload_event& e)
{
    LOG_INFO("upload progress {} {} {} {}", e.id, e.filename, e.upload_size, e.file_size);
}

int main(int argc, char* argv[])
{
    if (argc < 3)
    {
        std::cout << "usage: " << argv[0] << " upload_file download_file" << std::endl;
        return 0;
    }

    leaf::init_log("cli.log");

    leaf::set_log_level("trace");

    auto download = std::make_shared<leaf::download_session>("123", download_progress);

    auto upload = std::make_shared<leaf::upload_session>("456", upload_progress);

    boost::asio::io_context io(1);
    auto address = boost::asio::ip::address::from_string("127.0.0.1");
    boost::asio::ip::tcp::endpoint ed(address, 8080);
    auto downloader = std::make_shared<leaf::plain_websocket_client>("ws_cli", "/leaf/ws/download", download, ed, io);
    downloader->startup();
    auto uploader = std::make_shared<leaf::plain_websocket_client>("ws_cli", "/leaf/ws/upload", upload, ed, io);
    uploader->startup();

    std::vector<std::shared_ptr<leaf::file_context>> upload_files;
    {
        std::string filename = argv[1];
        auto file = std::make_shared<leaf::file_context>();
        file->file_path = filename;
        upload_files.push_back(file);
    }

    std::vector<std::shared_ptr<leaf::file_context>> download_files;
    {
        std::string filename = argv[2];
        auto file = std::make_shared<leaf::file_context>();
        file->file_path = filename;
        download_files.push_back(file);
    }
    boost::asio::steady_timer timer(io);
    timer.expires_after(std::chrono::seconds(1));
    timer.async_wait(
        [&](const boost::system::error_code&)
        {
            for (auto& file : download_files)
            {
                downloader->add_file(file);
            }
            for (auto& file : upload_files)
            {
                uploader->add_file(file);
            }
        });

    auto worker = boost::asio::make_work_guard(io);
    io.run();
    downloader->shutdown();
    uploader->shutdown();
    leaf::shutdown_log();
    return 0;
}
