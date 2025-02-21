#include <filesystem>
#include "log/log.h"
#include "crypt/random.h"
#include "net/http_client.h"
#include "file/file_transfer_client.h"

namespace leaf
{
static const char *upload_uri = "/leaf/ws/upload";
static const char *download_uri = "/leaf/ws/download";

file_transfer_client::file_transfer_client(const std::string &ip,
                                           uint16_t port,
                                           leaf::upload_progress_callback upload_progress_cb,
                                           leaf::download_progress_callback download_progress_cb,
                                           leaf::notify_progress_callback notify_progress_cb)
    : id_(leaf::random_string(8)),
      ed_(boost::asio::ip::address::from_string(ip), port),
      upload_progress_cb_(std::move(upload_progress_cb)),
      download_progress_cb_(std::move(download_progress_cb)),
      notify_progress_cb_(std::move(notify_progress_cb))
{
    login_url_ = "http://" + ip + ":" + std::to_string(ed_.port()) + "/leaf/login";
    download_url_ = "ws://" + ip + ":" + std::to_string(ed_.port()) + "/leaf/ws/download";
    upload_url_ = "ws://" + ip + ":" + std::to_string(ed_.port()) + "/leaf/ws/upload";
};

void file_transfer_client::do_login()
{
    auto c = std::make_shared<leaf::http_client>(executors.get_executor());
    c->post(login_url_,
            "",
            [this, c](boost::beast::error_code ec, const std::string &res)
            {
                on_login(ec, res);
                c->shutdown();
            });
}

void file_transfer_client::on_login(boost::beast::error_code ec, const std::string &res)
{
    if (ec)
    {
        LOG_ERROR("{} login failed {}", id_, ec.message());
        return;
    }
    token_ = res;
    LOG_INFO("login {} {} token {}", user_, pass_, token_);
    upload_->startup();
    download_->startup();
    upload_client_->startup();
    download_client_->startup();
    upload_->login(user_, pass_, token_);
    download_->login(user_, pass_, token_);

    start_timer();
}

void file_transfer_client::startup()
{
    executors.startup();
    timer_ = std::make_shared<boost::asio::steady_timer>(executors.get_executor());
    upload_ = std::make_shared<leaf::upload_session>("upload", upload_progress_cb_);
    download_ = std::make_shared<leaf::download_session>("download", download_progress_cb_, notify_progress_cb_);
    upload_->set_message_cb(std::bind(&file_transfer_client::on_write_upload_message, this, std::placeholders::_1));
    download_->set_message_cb(std::bind(&file_transfer_client::on_write_download_message, this, std::placeholders::_1));
    // clang-format off
    upload_client_ = std::make_shared<leaf::plain_websocket_client>("upload_ws_cli", upload_uri, ed_, executors.get_executor());
    download_client_ = std::make_shared<leaf::plain_websocket_client>("download_ws_cli", download_uri, ed_, executors.get_executor());
    upload_client_->set_message_handler(std::bind(&file_transfer_client::on_read_upload_message, this, std::placeholders::_1, std::placeholders::_2));
    download_client_->set_message_handler(std::bind(&file_transfer_client::on_read_download_message, this, std::placeholders::_1, std::placeholders::_2));
    // clang-format on
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
        on_error(ec);
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
        on_error(ec);
        return;
    }
    if (download_)
    {
        download_->on_message(*msg);
    }
}

void file_transfer_client::login(const std::string &user, const std::string &pass)
{
    user_ = user;
    pass_ = pass;
    do_login();
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

void file_transfer_client::on_error(const boost::system::error_code &ec)
{
    (void)ec;
    leaf::notify_event e;
    e.method = "logout";
    notify_progress_cb_(e);
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
