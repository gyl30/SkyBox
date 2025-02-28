#ifndef LEAF_FILE_COTROL_SESSION_H
#define LEAF_FILE_COTROL_SESSION_H

#include <memory>
#include "file/event.h"
#include "protocol/codec.h"

namespace leaf
{
class cotrol_session : public std::enable_shared_from_this<cotrol_session>
{
   public:
    cotrol_session(std::string id, leaf::cotrol_progress_callback cb);
    ~cotrol_session();

   public:
    void startup();
    void shutdown();
    void set_message_cb(std::function<void(std::vector<uint8_t>)> cb);
    void on_message(const std::vector<uint8_t> &msg);
    void on_message(const leaf::codec_message &msg);

   private:
    std::string id_;
    leaf::cotrol_progress_callback progress_cb_;
    std::function<void(std::vector<uint8_t>)> cb_;
};

}    // namespace leaf

#endif
