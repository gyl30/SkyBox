#include <iostream>
#include "log/log.h"
#include "file/event.h"
#include "file/file_transfer_client.h"

static void download_progress(const leaf::download_event &e)
{
    LOG_INFO("--> download progress {} {} {} {}", e.id, e.filename, e.download_size, e.file_size);
}

static void upload_progress(const leaf::upload_event &e)
{
    LOG_INFO("<-- upload progress {} {} {} {}", e.id, e.filename, e.upload_size, e.file_size);
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

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        std::cout << "usage: " << argv[0] << " upload_file download_file" << std::endl;
        return 0;
    }

    leaf::init_log("cli.log");

    leaf::set_log_level("trace");
    leaf::progress_handler handler;
    handler.download = download_progress;
    handler.upload = upload_progress;
    handler.notify = notify_progress;
    leaf::file_transfer_client fm("127.0.0.1", 8080, handler);
    fm.startup();

    fm.login("admin", "123456");

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    fm.add_upload_file(argv[1]);
    fm.add_download_file(argv[2]);

    std::this_thread::sleep_for(std::chrono::seconds(60));

    fm.shutdown();
    leaf::shutdown_log();
    return 0;
}
