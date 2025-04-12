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
    void add_file(const leaf::file_context::ptr &file);
    void on_message(const std::vector<uint8_t> &bytes);
    void on_message(const leaf::codec_message &msg);
    void set_message_cb(std::function<void(std::vector<uint8_t>)> cb);
    void login(const std::string &user, const std::string &pass, const leaf::login_token &l);

   private:
    void download_file_request();
    void on_download_file_response(const std::optional<leaf::download_file_response> &);
    void on_file_block_response(const std::optional<leaf::file_block_response> &);
    void block_data_request(uint32_t block_id);
    void on_block_data_response(const std::optional<leaf::block_data_response> &);
    void on_block_data_finish(const std::optional<leaf::block_data_finish> &);
    void on_error_response(const std::optional<leaf::error_message> &);
    void on_keepalive_response(const std::optional<leaf::keepalive> &);
    void on_login_response(const std::optional<leaf::login_response> &);


    void write_message(const codec_message &msg);
    void keepalive();
   private:
    void emit_event(const leaf::download_event &);
    void update_download_file();
    void reset();

   private:
    bool login_ = false;
    std::string id_;
    leaf::login_token token_;
    leaf::file_context::ptr file_;
    std::shared_ptr<leaf::blake2b> hash_;
    std::shared_ptr<leaf::writer> writer_;
    std::function<void(std::vector<uint8_t>)> cb_;
    leaf::download_progress_callback progress_cb_;
    leaf::notify_progress_callback notify_cb_;
    std::queue<leaf::file_context::ptr> padding_files_;
};
}    // namespace leaf

#endif
