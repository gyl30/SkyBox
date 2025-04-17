#ifndef LEAF_FILE_DOWNLOAD_SESSION_H
#define LEAF_FILE_DOWNLOAD_SESSION_H

#include <queue>
#include "file/file.h"
#include "file/event.h"
#include "protocol/codec.h"
#include "crypt/blake2b.h"
#include "file/file_context.h"

namespace leaf
{
class download_session : public std::enable_shared_from_this<download_session>
{
   public:
    download_session(std::string id, leaf::download_progress_callback cb, leaf::notify_progress_callback notify_cb);
    ~download_session();

   public:
    void startup();
    void shutdown();
    void update();

   public:
    void add_file(const std::string &file);
    void on_message(const std::vector<uint8_t> &bytes);
    void set_message_cb(std::function<void(std::vector<uint8_t>)> cb);
    void login(const std::string &token);

   private:
    void write_message(std::vector<uint8_t> bytes);
    void download_file_request();
    void on_download_file_response(const std::optional<leaf::download_file_response> &);
    void on_file_data(const std::optional<leaf::file_data> &);
    void on_error_message(const std::optional<leaf::error_message> &);
    void on_login_token(const std::optional<leaf::login_token> &message);
    void on_keepalive_response(const std::optional<leaf::keepalive> &);
    void keepalive();

   private:
    void emit_event(const leaf::download_event &);
    void reset();

   private:
    enum download_status : uint8_t
    {
        wait_download_file = 0,
        wait_file_data,
    };
    bool login_ = false;
    uint32_t seq_ = 0;
    std::string id_;
    leaf::login_token token_;
    download_status status_ = wait_download_file;
    leaf::file_context::ptr file_;
    std::shared_ptr<leaf::blake2b> hash_;
    std::shared_ptr<leaf::writer> writer_;
    std::function<void(std::vector<uint8_t>)> cb_;
    leaf::download_progress_callback progress_cb_;
    leaf::notify_progress_callback notify_cb_;
    std::queue<std::string> padding_files_;
};
}    // namespace leaf

#endif
