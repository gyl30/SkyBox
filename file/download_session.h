#ifndef LEAF_FILE_DOWNLOAD_SESSION_H
#define LEAF_FILE_DOWNLOAD_SESSION_H

#include "file/file.h"
#include "file/event.h"
#include "crypt/blake2b.h"
#include "protocol/message.h"
#include "file/file_context.h"
#include "net/plain_websocket_client.h"

namespace leaf
{
class download_session : public std::enable_shared_from_this<download_session>
{
   public:
    download_session(std::string id,
                     std::string token,
                     leaf::download_handle cb,
                     boost::asio::ip::tcp::endpoint ed_,
                     boost::asio::io_context &io);

    ~download_session();

   public:
    void startup();
    void shutdown();
    void update();

   public:
    void add_file(const std::string &file);
    void on_read(boost::beast::error_code ec, const std::vector<uint8_t> &bytes);
    void on_write(boost::beast::error_code ec, std::size_t transferred);
    void on_connect(boost::beast::error_code ec);

    void login(const std::string &token);

   private:
    void download_file_request();
    void on_download_file_response(const std::optional<leaf::download_file_response> &);
    void on_file_data(const std::optional<leaf::file_data> &);
    void on_error_message(const std::optional<leaf::error_message> &);
    void on_login_token(const std::optional<leaf::login_token> &message);
    void on_keepalive_response(const std::optional<leaf::keepalive> &);
    void keepalive();

   private:
    void safe_add_file(const std::string &filename);
    void safe_shutdown();
    void emit_event(const leaf::download_event &) const;
    void reset_state();

   private:
    enum download_status : uint8_t
    {
        wait_download_file = 0,
        wait_file_data,
    };
    bool login_ = false;
    uint32_t seq_ = 0;
    std::string id_;
    std::string token_;
    boost::asio::io_context &io_;
    boost::asio::ip::tcp::endpoint ed_;
    download_status status_ = wait_download_file;
    leaf::file_context::ptr file_;
    std::shared_ptr<leaf::blake2b> hash_;
    std::shared_ptr<leaf::writer> writer_;
    std::queue<std::string> padding_files_;
    leaf::download_handle progress_cb_;
    std::shared_ptr<leaf::plain_websocket_client> ws_client_;
};
}    // namespace leaf

#endif
