#ifndef LEAF_FILE_FILE_TRANSFER_CLIENT_H
#define LEAF_FILE_FILE_TRANSFER_CLIENT_H

#include "file/event.h"
#include "net/executors.h"
#include "file/upload_session.h"
#include "file/download_session.h"
#include "net/plain_websocket_client.h"

namespace leaf
{

class file_transfer_client
{
   public:
    file_transfer_client(const std::string &ip,
                         uint16_t port,
                         leaf::upload_progress_callback upload_progress_cb,
                         leaf::download_progress_callback download_progress_cb);

   public:
    void startup();
    void shutdown();

   public:
    void login(const std::string &user, const std::string &pass);
    void add_upload_file(const std::string &filename);
    void add_download_file(const std::string &filename);

   private:
    void start_timer();
    void timer_callback(const boost::system::error_code &ec);
    void upload_message(const std::shared_ptr<std::vector<uint8_t>> &msg, const boost::system::error_code &ec);
    void download_message(const std::shared_ptr<std::vector<uint8_t>> &msg, const boost::system::error_code &ec);

   private:
    std::string id_;
    leaf::executors executors{4};
    boost::asio::ip::tcp::endpoint ed_;
    std::shared_ptr<leaf::upload_session> upload_;
    std::shared_ptr<leaf::download_session> download_;
    std::shared_ptr<boost::asio::steady_timer> timer_;
    leaf::upload_progress_callback upload_progress_cb_;
    leaf::download_progress_callback download_progress_cb_;
    std::shared_ptr<leaf::plain_websocket_client> upload_client_;
    std::shared_ptr<leaf::plain_websocket_client> download_client_;
};

}    // namespace leaf

#endif
