#include <filesystem>
#include "log/log.h"
#include "file/file_transfer_client.h"

namespace leaf
{
static const char *upload_uri = "/leaf/ws/upload";
static const char *download_uri = "/leaf/ws/download";

file_transfer_client::file_transfer_client(const std::string &ip,
                                           uint16_t port,
                                           leaf::upload_progress_callback upload_progress_cb,
                                           leaf::download_progress_callback download_progress_cb)
    : ed_(boost::asio::ip::address::from_string(ip), port),
      upload_progress_cb_(std::move(upload_progress_cb)),
      download_progress_cb_(std::move(download_progress_cb)) {};

void file_transfer_client::startup()
{
    executors.startup();
    timer_ = std::make_shared<boost::asio::steady_timer>(executors.get_executor());
    upload_ = std::make_shared<leaf::upload_session>("upload", upload_progress_cb_);
    download_ = std::make_shared<leaf::download_session>("download", download_progress_cb_);
    upload_->set_message_cb(std::bind(&file_transfer_client::on_write_upload_message, this, std::placeholders::_1));
    download_->set_message_cb(std::bind(&file_transfer_client::on_write_download_message, this, std::placeholders::_1));
    upload_->startup();
    download_->startup();
    // clang-format off
    upload_client_ = std::make_shared<leaf::plain_websocket_client>("upload_ws_cli", upload_uri, ed_, executors.get_executor());
    download_client_ = std::make_shared<leaf::plain_websocket_client>("download_ws_cli", download_uri, ed_, executors.get_executor());
    upload_client_->set_message_handler(std::bind(&file_transfer_client::on_read_upload_message, this, std::placeholders::_1, std::placeholders::_2));
    download_client_->set_message_handler(std::bind(&file_transfer_client::on_read_download_message, this, std::placeholders::_1, std::placeholders::_2));
    // clang-format on
    upload_client_->startup();
    download_client_->startup();

    start_timer();
}
void file_transfer_client::shutdown()
{
    boost::system::error_code ec;
    timer_->cancel(ec);
    upload_->shutdown();
    download_->shutdown();
    upload_client_->shutdown();
    download_client_->shutdown();
    upload_.reset();
    download_.reset();
    upload_client_.reset();
    download_client_.reset();
    executors.shutdown();
}
void file_transfer_client::on_write_upload_message(std::vector<uint8_t> msg)
{
    if (upload_client_)
    {
        upload_client_->write(std::move(msg));
    }
}
void file_transfer_client::on_write_download_message(std::vector<uint8_t> msg)
{
    if (download_client_)
    {
        download_client_->write(std::move(msg));
    }
}

void file_transfer_client::on_read_upload_message(const std::shared_ptr<std::vector<uint8_t>> &msg,
                                                  const boost::system::error_code &ec)
{
    if (ec)
    {
        LOG_ERROR("{} upload read error {}", id_, ec.message());
        return;
    }
    if (upload_)
    {
        upload_->on_message(*msg);
    }
}
void file_transfer_client::on_read_download_message(const std::shared_ptr<std::vector<uint8_t>> &msg,
                                                    const boost::system::error_code &ec)
{
    if (ec)
    {
        LOG_ERROR("{} download read error {}", id_, ec.message());
        return;
    }
    if (download_)
    {
        download_->on_message(*msg);
    }
}

void file_transfer_client::login(const std::string &user, const std::string &pass)
{
    upload_->login(user, pass);
    download_->login(user, pass);
}
void file_transfer_client::add_upload_file(const std::string &filename)
{
    auto file = std::make_shared<leaf::file_context>();
    file->file_path = filename;
    file->filename = std::filesystem::path(filename).filename();
    upload_->add_file(file);
}
void file_transfer_client::add_download_file(const std::string &filename)
{
    auto file = std::make_shared<leaf::file_context>();
    file->file_path = filename;
    download_->add_file(file);
}
void file_transfer_client::start_timer()
{
    timer_->expires_after(std::chrono::seconds(1));
    timer_->async_wait(std::bind(&file_transfer_client::timer_callback, this, std::placeholders::_1));
    //
}
void file_transfer_client::timer_callback(const boost::system::error_code &ec)
{
    if (ec)
    {
        LOG_ERROR("{} timer error {}", id_, ec.message());
        return;
    }

    upload_->update();
    download_->update();

    start_timer();
}

}    // namespace leaf
