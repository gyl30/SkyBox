#include <iostream>
#include <utility>

#include "log.h"
#include "event.h"
#include "file_transfer_client.h"

static void download_progress(const leaf::download_event &e)
{
    LOG_INFO("--> download progress {} {} {} {}", e.id, e.filename, e.download_size, e.file_size);
}

static void upload_progress(const leaf::upload_event &e)
{
    LOG_INFO("<-- upload progress {} {} {} {}", e.id, e.filename, e.upload_size, e.file_size);
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

    leaf::file_manager fm("127.0.0.1", 8080, upload_progress, download_progress);
    fm.startup();

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    fm.add_upload_file(argv[1]);
    fm.add_download_file(argv[2]);

    std::this_thread::sleep_for(std::chrono::seconds(60));

    fm.shutdown();
    leaf::shutdown_log();
    return 0;
}
