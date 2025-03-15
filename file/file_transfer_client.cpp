#include <filesystem>
#include <utility>
#include "log/log.h"
#include "crypt/random.h"
#include "net/http_client.h"
#include "protocol/message.h"
#include "file/file_transfer_client.h"

namespace leaf
{
static const char *upload_uri = "/leaf/ws/upload";
static const char *download_uri = "/leaf/ws/download";
static const char *cotrol_uri = "/leaf/ws/cotrol";

file_transfer_client::file_transfer_client(const std::string &ip, uint16_t port, leaf::progress_handler handler)
    : id_(leaf::random_string(8)), ed_(boost::asio::ip::address::from_string(ip), port), handler_(std::move(handler))
{
    login_url_ = "http://" + ip + ":" + std::to_string(ed_.port()) + "/leaf/login";
    download_url_ = "ws://" + ip + ":" + std::to_string(ed_.port()) + "/leaf/ws/download";
    upload_url_ = "ws://" + ip + ":" + std::to_string(ed_.port()) + "/leaf/ws/upload";
};

void file_transfer_client::do_login()
{
    if (!cotrol_connect_ || !upload_connect_ || !download_connect_)
    {
        return;
    }
    auto c = std::make_shared<leaf::http_client>(executors.get_executor());
    leaf::login_request l;
    l.username = user_;
    l.password = pass_;
    auto data = leaf::message::serialize_login_request(l);
    c->post(login_url_,
            std::string(data.begin(), data.end()),
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
    auto l = leaf::deserialize_message(res);
    if (!l)
    {
        LOG_ERROR("{} login deserialize failed {}", id_, res);
        return;
    }
    login_ = true;
    token_ = l->token;
    LOG_INFO("login {} {} token {} block {}", user_, pass_, token_, l->block_size);
    upload_->login(user_, pass_, l.value());
    download_->login(user_, pass_, l.value());

    start_timer();
}

void file_transfer_client::startup()
{
    executors.startup();
    timer_ = std::make_shared<boost::asio::steady_timer>(executors.get_executor());
    cotrol_ = std::make_shared<leaf::cotrol_session>("cotrol", handler_.cotrol);
    upload_ = std::make_shared<leaf::upload_session>("upload", handler_.upload);
    download_ = std::make_shared<leaf::download_session>("download", handler_.download, handler_.notify);
    cotrol_->set_message_cb(std::bind(&file_transfer_client::on_write_cotrol_message, this, std::placeholders::_1));
    upload_->set_message_cb(std::bind(&file_transfer_client::on_write_upload_message, this, std::placeholders::_1));
    download_->set_message_cb(std::bind(&file_transfer_client::on_write_download_message, this, std::placeholders::_1));
    // clang-format off
    cotrol_client_ = std::make_shared<leaf::plain_websocket_client>("cotrol_ws_cli", cotrol_uri, ed_, executors.get_executor());
    upload_client_ = std::make_shared<leaf::plain_websocket_client>("upload_ws_cli", upload_uri, ed_, executors.get_executor());
    download_client_ = std::make_shared<leaf::plain_websocket_client>("download_ws_cli", download_uri, ed_, executors.get_executor());
    cotrol_client_->set_message_handler(std::bind(&file_transfer_client::on_read_cotrol_message, this, std::placeholders::_1, std::placeholders::_2));
    upload_client_->set_message_handler(std::bind(&file_transfer_client::on_read_upload_message, this, std::placeholders::_1, std::placeholders::_2));
    download_client_->set_message_handler(std::bind(&file_transfer_client::on_read_download_message, this, std::placeholders::_1, std::placeholders::_2));
    cotrol_client_->set_handshake_handler(std::bind(&file_transfer_client::on_cotrol_connect, this, std::placeholders::_1));
    upload_client_->set_handshake_handler(std::bind(&file_transfer_client::on_upload_connect, this, std::placeholders::_1));
    download_client_->set_handshake_handler(std::bind(&file_transfer_client::on_download_connect, this, std::placeholders::_1));
    // clang-format on
    cotrol_->startup();
    upload_->startup();
    download_->startup();
    cotrol_client_->startup();
    upload_client_->startup();
    download_client_->startup();
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
void file_transfer_client::on_write_cotrol_message(std::vector<uint8_t> msg)
{
    if (cotrol_client_)
    {
        cotrol_client_->write(std::move(msg));
    }
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

void file_transfer_client::on_read_cotrol_message(const std::shared_ptr<std::vector<uint8_t>> &msg,
                                                  const boost::system::error_code &ec)
{
    if (ec)
    {
        LOG_ERROR("{} cotrol read error {}", id_, ec.message());
        on_error(ec);
        return;
    }
    if (cotrol_)
    {
        cotrol_->on_message(*msg);
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

void file_transfer_client::on_cotrol_connect(const boost::system::error_code &ec)
{
    if (ec)
    {
        return;
    }
    cotrol_connect_ = true;
    LOG_INFO("{} cotrol connect success", id_);
    do_login();
}
void file_transfer_client::on_upload_connect(const boost::system::error_code &ec)
{
    if (ec)
    {
        return;
    }
    LOG_INFO("{} upload connect success", id_);
    upload_connect_ = true;
    do_login();
}
void file_transfer_client::on_download_connect(const boost::system::error_code &ec)
{
    if (ec)
    {
        return;
    }
    LOG_INFO("{} download connect success", id_);
    download_connect_ = true;
    do_login();
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
    file->filename = std::filesystem::path(filename).filename();
    download_->add_file(file);
}

void file_transfer_client::on_error(const boost::system::error_code &ec) const
{
    (void)ec;
    leaf::notify_event e;
    e.method = "logout";
    handler_.notify(e);
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

    if (!login_)
    {
        do_login();
        return;
    }
    upload_->update();
    download_->update();

    start_timer();
}

}    // namespace leaf
