#ifndef LEAF_FILE_COTROL_SESSION_H
#define LEAF_FILE_COTROL_SESSION_H

#include <memory>
#include <optional>
#include "file/event.h"
#include "protocol/codec.h"

namespace leaf
{
class cotrol_session : public std::enable_shared_from_this<cotrol_session>
{
   public:
    cotrol_session(std::string id, leaf::cotrol_progress_callback cb, leaf::notify_progress_callback notify_cb);
    ~cotrol_session();

   public:
    void startup();
    void shutdown();
    void update();
    void login(const std::string &user, const std::string &pass, const leaf::login_token &l);
    void set_message_cb(std::function<void(std::vector<uint8_t>)> cb);
    void on_message(const std::vector<uint8_t> &bytes);

   private:
    void files_request();
    void write_message(const codec_message &msg);
    void on_files_response(const std::optional<leaf::files_response> &);

   private:
    std::string id_;
    leaf::login_token token_;
    leaf::notify_progress_callback notify_cb_;
    leaf::cotrol_progress_callback progress_cb_;
    std::function<void(std::vector<uint8_t>)> cb_;
};

}    // namespace leaf

#endif
