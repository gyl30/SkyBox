#ifndef LEAF_FILE_UPLOAD_SESSION_H
#define LEAF_FILE_UPLOAD_SESSION_H

#include <deque>
#include "file/file.h"
#include "file/event.h"
#include "protocol/codec.h"
#include "crypt/blake2b.h"
#include "file/file_context.h"
#include "net/plain_websocket_client.h"

namespace leaf
{
class upload_session : public std::enable_shared_from_this<upload_session>
{
   public:
    upload_session(std::string id,
                   leaf::upload_progress_callback cb,
                   boost::asio::ip::tcp::endpoint ed,
                   boost::asio::io_context &io);

    ~upload_session();

   public:
    void startup();
    void shutdown();
    void update();

   public:
    void add_file(const leaf::file_context::ptr &file);
    void on_message(const std::vector<uint8_t> &bytes);
    void set_message_cb(std::function<void(std::vector<uint8_t>)> cb);
    void login(const std::string &token);

   private:
    void update_process_file();
    void upload_file_request();
    void keepalive();
    void padding_file_event();
    void on_keepalive_response(const std::optional<leaf::keepalive> &);
    void on_upload_file_response(const std::optional<leaf::upload_file_response> &);
    void on_error_message(const std::optional<leaf::error_message> &);
    void on_login_token(const std::optional<leaf::login_token> &l);
    void update_file_data();
    void emit_event(const leaf::upload_event &e);

   private:
    bool login_ = false;
    std::string id_;
    uint32_t seq_ = 0;
    leaf::login_token token_;
    boost::asio::io_context &io_;
    leaf::file_context::ptr file_;
    boost::asio::ip::tcp::endpoint ed_;
    std::shared_ptr<leaf::reader> reader_;
    std::shared_ptr<leaf::blake2b> hash_;
    leaf::upload_progress_callback progress_cb_;
    std::function<void(std::vector<uint8_t>)> cb_;
    std::deque<leaf::file_context::ptr> padding_files_;
    std::shared_ptr<leaf::plain_websocket_client> ws_client_;
};
}    // namespace leaf

#endif
