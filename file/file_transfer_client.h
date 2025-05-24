#ifndef LEAF_FILE_FILE_TRANSFER_CLIENT_H
#define LEAF_FILE_FILE_TRANSFER_CLIENT_H

#include "file/event.h"
#include "net/executors.h"
#include "file/cotrol_session.h"
#include "file/upload_session.h"
#include "file/download_session.h"

namespace leaf
{

class file_transfer_client
{
   public:
    file_transfer_client(std::string ip, uint16_t port, leaf::progress_handler handler);

    ~file_transfer_client();

   public:
    void startup();
    void shutdown();

   public:
    void login(const std::string &user, const std::string &pass);
    void add_upload_file(const std::string &filename);
    void add_download_file(const std::string &filename);

    void add_upload_files(const std::vector<std::string> &files);
    void add_download_files(const std::vector<std::string> &files);
    void create_directory(const std::string &dir);
    void change_current_dir(const std::string &dir);

   private:
    void do_login();
    void on_login(boost::beast::error_code ec, const std::string &res);

    void safe_shutdown();
    void start_timer();
    void timer_callback(const boost::system::error_code &ec);
    void on_write_cotrol_message(std::vector<uint8_t> msg);
    void on_write_upload_message(std::vector<uint8_t> msg);
    void on_write_download_message(std::vector<uint8_t> msg);
    void on_read_cotrol_message(const std::shared_ptr<std::vector<uint8_t>> &msg, const boost::system::error_code &ec);
    void on_read_upload_message(const std::shared_ptr<std::vector<uint8_t>> &msg, const boost::system::error_code &ec);
    void on_read_download_message(const std::shared_ptr<std::vector<uint8_t>> &msg, const boost::system::error_code &ec);

   private:
    std::string id_;
    std::string host_;
    std::string port_;
    std::string token_;
    std::string user_;
    std::string pass_;
    bool login_ = false;
    leaf::executors executors{4};
    std::once_flag shutdown_flag_;
    leaf::progress_handler handler_;
    boost::asio::ip::tcp::endpoint ed_;
    std::shared_ptr<leaf::cotrol_session> cotrol_;
    std::shared_ptr<leaf::upload_session> upload_;
    leaf::executors::executor *ex_;
    std::shared_ptr<leaf::download_session> download_;
    std::shared_ptr<boost::asio::steady_timer> timer_;
};

}    // namespace leaf

#endif
