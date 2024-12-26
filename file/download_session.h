#ifndef LEAF_FILE_DOWNLOAD_SESSION_H
#define LEAF_FILE_DOWNLOAD_SESSION_H

#include <queue>
#include "file/file.h"
#include "file/event.h"
#include "protocol/codec.h"
#include "crypt/blake2b.h"
#include "file/file_context.h"
#include "net/base_session.h"

namespace leaf
{
class download_session : public base_session, public std::enable_shared_from_this<download_session>
{
   public:
    download_session(std::string id, leaf::download_progress_callback cb);
    ~download_session() override;

   public:
    void startup() override;
    void shutdown() override;
    void update() override;

   public:
    void add_file(const leaf::file_context::ptr &file) override;
    void on_message(const leaf::codec_message &msg) override;
    void set_message_cb(std::function<void(const leaf::codec_message &)> cb) override;
    void login(const std::string &user, const std::string &pass);

   private:
    void download_file_request();
    void on_download_file_response(const leaf::download_file_response &);
    void on_file_block_response(const leaf::file_block_response &);
    void block_data_request(uint32_t block_id);
    void on_block_data_response(const leaf::block_data_response &);
    void on_block_data_finish(const leaf::block_data_finish &);
    void error_response(const leaf::error_response &);
    void write_message(const codec_message &msg);
    void keepalive();
    void keepalive_response(const leaf::keepalive &);
    void login_response(const leaf::login_response &);

   private:
    void emit_event(const leaf::download_event &);
    void update_download_file();


   private:
    bool login_ = false;
    std::string id_;
    leaf::file_context::ptr file_;
    std::shared_ptr<leaf::blake2b> hash_;
    std::shared_ptr<leaf::writer> writer_;
    leaf::download_progress_callback progress_cb_;
    std::queue<leaf::file_context::ptr> padding_files_;
    std::function<void(const leaf::codec_message &)> cb_;
};
}    // namespace leaf

#endif
