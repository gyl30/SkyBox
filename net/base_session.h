#ifndef LEAF_CLI_BASE_SESSION_H
#define LEAF_CLI_BASE_SESSION_H

#include "codec.h"
#include "file_context.h"

namespace leaf
{
class base_session
{
   public:
    base_session() = default;
    virtual ~base_session() = default;

   public:
    virtual void startup() = 0;
    virtual void shutdown() = 0;
    virtual void update() = 0;

   public:
    virtual void add_file(const leaf::file_context::ptr &file) = 0;
    virtual void on_message(const leaf::codec_message &msg) = 0;
    virtual void set_message_cb(std::function<void(const leaf::codec_message &)> cb) = 0;
};
}    // namespace leaf

#endif
