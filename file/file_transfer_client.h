#ifndef LEAF_FILE_FILE_TRANSFER_CLIENT_H
#define LEAF_FILE_FILE_TRANSFER_CLIENT_H

#include "file/event.h"
#include "net/executors.h"
#include "file/cotrol_session.h"
#include "file/upload_session.h"
#include "file/download_session.h"

namespace leaf
{

class file_transfer_client : public std::enable_shared_from_this<file_transfer_client>
{
   public:
    file_transfer_client(std::string ip, uint16_t port, std::string username, std::string password);

    ~file_transfer_client();

   public:
    void startup();
    void shutdown();

   public:
    void add_upload_file(const std::string &filename);
    void add_download_file(const std::string &filename);
    void add_upload_files(const std::vector<std::string> &files);
    void add_download_files(const std::vector<std::string> &files);
    void create_directory(const std::string &dir);
    void change_current_dir(const std::string &dir);

   private:
    boost::asio::awaitable<void> start_coro();
    boost::asio::awaitable<void> login(boost::system::error_code &ec);
    boost::asio::awaitable<void> shutdown_coro();

   private:
    std::string id_;
    std::string host_;
    std::string port_;
    std::string token_;
    std::string user_;
    std::string pass_;
    leaf::executors executors{4};
    std::once_flag shutdown_flag_;
    leaf::executors::executor *ex_;
    std::shared_ptr<leaf::cotrol_session> cotrol_;
    std::shared_ptr<leaf::upload_session> upload_;
    std::shared_ptr<leaf::download_session> download_;
};

}    // namespace leaf

#endif
