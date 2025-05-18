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
                   leaf::cotrol_handle handler,
                   boost::asio::ip::tcp::endpoint ed,
                   boost::asio::io_context &io);
    ~cotrol_session();

   public:
    void startup();
    void shutdown();
    void update();
    void create_directory(const std::string &dir);
    void change_current_dir(const std::string &dir);

   private:
    void files_request();
    void on_read(boost::beast::error_code ec, const std::vector<uint8_t> &bytes);
    void on_write(boost::beast::error_code ec, std::size_t transferred);
    void on_connect(boost::beast::error_code ec);
    void on_files_response(const std::optional<leaf::files_response> &files);

   private:
    std::string id_;
    uint32_t seq_ = 0;
    std::string token_;
    std::string current_dir_;
    boost::asio::io_context &io_;
    boost::asio::ip::tcp::endpoint ed_;
    leaf::cotrol_handle handler_;
    std::shared_ptr<leaf::plain_websocket_client> ws_client_;
};

}    // namespace leaf

#endif
