#ifndef LEAF_FILE_FILE_TRANSFER_CLIENT_H
#define LEAF_FILE_FILE_TRANSFER_CLIENT_H

#include "file/event.h"
#include "net/executors.h"
#include "file/cotrol_session.h"
#include "file/upload_session.h"
#include "file/download_session.h"
#include "net/plain_websocket_client.h"

namespace leaf
{

class file_transfer_client
{
   public:
    file_transfer_client(const std::string &ip, uint16_t port, leaf::progress_handler handler);

    ~file_transfer_client();

   public:
    void startup();
    void shutdown();

   public:
    void login(const std::string &user, const std::string &pass);
    void add_upload_file(const std::string &filename);
    void add_download_file(const std::string &filename);

   private:
    void do_login();
    void on_login(boost::beast::error_code ec, const std::string &res);
    void on_login_failed() const;
    void on_login_success() const;

    void start_timer();
    void timer_callback(const boost::system::error_code &ec);
    void on_write_cotrol_message(std::vector<uint8_t> msg);
    void on_write_upload_message(std::vector<uint8_t> msg);
    void on_write_download_message(std::vector<uint8_t> msg);
    void on_read_cotrol_message(const std::shared_ptr<std::vector<uint8_t>> &msg, const boost::system::error_code &ec);
    void on_read_upload_message(const std::shared_ptr<std::vector<uint8_t>> &msg, const boost::system::error_code &ec);
    void on_read_download_message(const std::shared_ptr<std::vector<uint8_t>> &msg,
                                  const boost::system::error_code &ec);
    void on_error(const boost::system::error_code &ec) const;
    void on_cotrol_connect(const boost::system::error_code &ec);
    void on_upload_connect(const boost::system::error_code &ec);
    void on_download_connect(const boost::system::error_code &ec);

   private:
    std::string id_;
    std::string token_;
    std::string user_;
    std::string pass_;
    std::string login_url_;
    std::string upload_url_;
    std::string download_url_;
    bool login_ = false;
    bool cotrol_connect_ = false;
    bool upload_connect_ = false;
    bool download_connect_ = false;
    leaf::executors executors{4};
    boost::asio::ip::tcp::endpoint ed_;
    leaf::progress_handler handler_;
    std::shared_ptr<leaf::cotrol_session> cotrol_;
    std::shared_ptr<leaf::upload_session> upload_;
    std::shared_ptr<leaf::download_session> download_;
    std::shared_ptr<boost::asio::steady_timer> timer_;
    std::shared_ptr<leaf::plain_websocket_client> cotrol_client_;
    std::shared_ptr<leaf::plain_websocket_client> upload_client_;
    std::shared_ptr<leaf::plain_websocket_client> download_client_;
};

}    // namespace leaf

#endif
