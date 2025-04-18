#ifndef LEAF_FILE_COTROL_SESSION_H
#define LEAF_FILE_COTROL_SESSION_H

#include <memory>
#include <optional>
#include "file/event.h"
#include "protocol/codec.h"
#include "net/plain_websocket_client.h"

namespace leaf
{
class cotrol_session : public std::enable_shared_from_this<cotrol_session>
{
   public:
    cotrol_session(std::string id,
                   std::string token,
                   leaf::cotrol_progress_callback cb,
                   leaf::notify_progress_callback notify_cb,
                   boost::asio::ip::tcp::endpoint ed,
                   boost::asio::io_context &io);
    ~cotrol_session();

   public:
    void startup();
    void shutdown();
    void update();
    void on_read(boost::beast::error_code ec, const std::vector<uint8_t> &bytes);
    void on_write(boost::beast::error_code ec, std::size_t transferred);
    void on_connect(boost::beast::error_code ec);
 
   private:
    void files_request();
    void on_files_response(const std::optional<leaf::files_response> &);

   private:
    std::string id_;
    uint32_t seq_ = 0;
    std::string token_;
    boost::asio::io_context &io_;
    boost::asio::ip::tcp::endpoint ed_;
    leaf::notify_progress_callback notify_cb_;
    leaf::cotrol_progress_callback progress_cb_;
    std::shared_ptr<leaf::plain_websocket_client> ws_client_;
};

}    // namespace leaf

#endif
