#include "log/log.h"
#include "crypt/random.h"
#include "net/http_client.h"
#include "protocol/message.h"
#include "file/file_transfer_client.h"

namespace leaf
{

file_transfer_client::file_transfer_client(const std::string &ip, uint16_t port, leaf::progress_handler handler)
    : id_(leaf::random_string(8)), handler_(std::move(handler)), ed_(boost::asio::ip::address::from_string(ip), port)
{
}

file_transfer_client::~file_transfer_client()
{
    //
    LOG_INFO("{} shutdown", id_);
}
void file_transfer_client::do_login()
{
    auto c = std::make_shared<leaf::http_client>(executors.get_executor());
    leaf::login_request l;
    l.username = user_;
    l.password = pass_;
    std::string login_url = "http://" + ed_.address().to_string() + ":" + std::to_string(ed_.port()) + "/leaf/login";
    auto data = leaf::serialize_login_request(l);
    c->post(login_url,
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
    if (login_)
    {
        return;
    }
    std::vector<uint8_t> data(res.begin(), res.end());
    auto l = leaf::deserialize_login_token(data);
    if (!l)
    {
        LOG_ERROR("{} login deserialize failed {}", id_, res);
        return;
    }

    login_ = true;
    token_ = l->token;
    LOG_INFO("login {} {} token {}", user_, pass_, token_, l->token);
    // clang-format off
    cotrol_ = std::make_shared<leaf::cotrol_session>("cotrol", l->token, handler_.c,  ed_, executors.get_executor());
    upload_ = std::make_shared<leaf::upload_session>("upload", l->token, handler_.u, ed_, executors.get_executor());
    download_ = std::make_shared<leaf::download_session>("download", l->token, handler_.d, ed_, executors.get_executor());
    // clang-format on
    cotrol_->startup();
    upload_->startup();
    download_->startup();
    start_timer();
}

void file_transfer_client::startup()
{
    executors.startup();
    ex_ = &executors.get_executor();
    timer_ = std::make_shared<boost::asio::steady_timer>(*ex_);
}

void file_transfer_client::shutdown()
{
    std::call_once(shutdown_flag_, [this]() { safe_shutdown(); });
}
void file_transfer_client::safe_shutdown()
{
    boost::system::error_code ec;
    timer_->cancel(ec);
    if (upload_)
    {
        upload_->shutdown();
        upload_.reset();
    }
    if (download_)
    {
        download_->shutdown();
        download_.reset();
    }
    if (cotrol_)
    {
        cotrol_->shutdown();
        cotrol_.reset();
    }
    executors.shutdown();
}

void file_transfer_client::login(const std::string &user, const std::string &pass)
{
    ex_->post(
        [user, pass, this]()
        {
            user_ = user;
            pass_ = pass;
            do_login();
        });
}
void file_transfer_client::add_upload_file(const std::string &filename)
{
    ex_->post(
        [this, filename]()
        {
            if (upload_)
            {
                upload_->add_file(filename);
            }
        });
}
void file_transfer_client::add_download_file(const std::string &filename)
{
    ex_->post(
        [this, filename]()
        {
            if (download_)
            {
                download_->add_file(filename);
            }
        });
}
void file_transfer_client::add_upload_files(const std::vector<std::string> &files)
{
    ex_->post(
        [this, files]()
        {
            if (upload_)
            {
                upload_->add_files(files);
            }
        });
}
void file_transfer_client::add_download_files(const std::vector<std::string> &files)
{
    ex_->post(
        [this, files]()
        {
            if (download_)
            {
                download_->add_files(files);
            }
        });
}

void file_transfer_client::change_current_dir(const std::string &dir)
{
    if (cotrol_)
    {
        cotrol_->change_current_dir(dir);
    }
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
    if (upload_)
    {
        upload_->update();
    }
    if (download_)
    {
        download_->update();
    }
    if (cotrol_)
    {
        cotrol_->update();
    }

    start_timer();
}

}    // namespace leaf
