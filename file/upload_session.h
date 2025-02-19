#ifndef LEAF_FILE_UPLOAD_SESSION_H
#define LEAF_FILE_UPLOAD_SESSION_H

#include <deque>
#include "file/file.h"
#include "file/event.h"
#include "protocol/codec.h"
#include "crypt/blake2b.h"
#include "file/file_context.h"

namespace leaf
{
class upload_session : public std::enable_shared_from_this<upload_session>
{
   public:
    upload_session(std::string id, leaf::upload_progress_callback cb);
    ~upload_session();

   public:
    void startup();
    void shutdown();
    void update();

   public:
    void add_file(const leaf::file_context::ptr &file);
    void on_message(std::vector<uint8_t> msg);
    void on_message(const leaf::codec_message &msg);
    void set_message_cb(std::function<void(std::vector<uint8_t>)> cb);
    void login(const std::string &user, const std::string &pass, const std::string &token);

   private:
    void open_file();
    void update_process_file();
    void upload_file_request();
    void on_upload_file_response(const leaf::upload_file_response &);
    void on_upload_file_exist(const leaf::upload_file_exist &);
    void on_delete_file_response(const leaf::delete_file_response &);
    void on_block_data_request(const leaf::block_data_request &);
    void on_file_block_request(const leaf::file_block_request &);
    void on_block_data_finish(const leaf::block_data_finish &);
    void keepalive();
    void padding_file_event();
    void on_login_response(const leaf::login_response &);
    void on_keepalive_response(const leaf::keepalive &);
    void on_error_response(const leaf::error_response &);
    void write_message(const codec_message &msg);
    void emit_event(const leaf::upload_event &e);

   private:
    bool login_ = false;
    std::string id_;
    std::string token_;
    leaf::file_context::ptr file_;
    std::shared_ptr<leaf::reader> reader_;
    std::shared_ptr<leaf::blake2b> blake2b_;
    leaf::upload_progress_callback progress_cb_;
    std::function<void(std::vector<uint8_t>)> cb_;
    std::deque<leaf::file_context::ptr> padding_files_;
};
}    // namespace leaf

#endif
