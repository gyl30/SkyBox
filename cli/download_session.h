#ifndef LEAF_DOWNLOAD_SESSION_H
#define LEAF_DOWNLOAD_SESSION_H

#include <queue>
#include "codec.h"
#include "blake2b.h"
#include "file_context.h"

namespace leaf
{
class download_session
{
   public:
    explicit download_session(std::string id);
    ~download_session();

   public:
    void startup();
    void shutdown();
    void update();

   public:
    void add_file(const leaf::file_context::ptr &file);
    void on_message(const leaf::codec_message &msg);
    void set_message_cb(std::function<void(const leaf::codec_message &)> cb);

   private:
    void open_file();
    void download_file_request();
    void download_file_response(const leaf::download_file_response &);
    void error_response(const leaf::error_response &);
    void write_message(const codec_message &msg);

   private:
    std::string id_;
    leaf::file_context::ptr file_;
    std::shared_ptr<leaf::writer> writer_;
    std::shared_ptr<leaf::blake2b> blake2b_;
    std::queue<leaf::file_context::ptr> padding_files_;
    std::function<void(const leaf::codec_message &)> cb_;
};
}    // namespace leaf

#endif
