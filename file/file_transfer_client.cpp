#include "file_transfer_client.h"

namespace leaf
{
static const char *upload_uri = "/leaf/ws/upload";
static const char *download_uri = "/leaf/ws/download";

file_manager::file_manager(const std::string &ip,
                           uint16_t port,
                           leaf::upload_progress_callback upload_progress_cb,
                           leaf::download_progress_callback download_progress_cb)
    : ed_(boost::asio::ip::address::from_string(ip), port),
      upload_progress_cb_(std::move(upload_progress_cb)),
      download_progress_cb_(std::move(download_progress_cb)) {};

void file_manager::startup()
{
    executors.startup();
    upload = std::make_shared<leaf::upload_session>("upload", upload_progress_cb_);
    download = std::make_shared<leaf::download_session>("download", download_progress_cb_);
    upload->startup();
    download->startup();
    // clang-format off
        upload_client_ = std::make_shared<leaf::plain_websocket_client>("ws_cli", upload_uri, upload, ed_, executors.get_executor());
        download_client_ = std::make_shared<leaf::plain_websocket_client>("ws_cli", download_uri, download, ed_, executors.get_executor());
    // clang-format on
    upload_client_->startup();
    download_client_->startup();
}
void file_manager::shutdown()
{
    upload_client_->shutdown();
    download_client_->shutdown();
    upload->shutdown();
    download->shutdown();
    executors.shutdown();
}

void file_manager::add_upload_file(const std::string &filename)
{
    auto file = std::make_shared<leaf::file_context>();
    file->file_path = filename;
    upload->add_file(file);
}
void file_manager::add_download_file(const std::string &filename)
{
    auto file = std::make_shared<leaf::file_context>();
    file->file_path = filename;
    download->add_file(file);
}

}    // namespace leaf
