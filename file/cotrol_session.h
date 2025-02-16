#ifndef LEAF_FILE_COTROL_SESSION_H
#define LEAF_FILE_COTROL_SESSION_H

#include <memory>
#include "file/event.h"

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

   private:
    std::string id_;
    leaf::cotrol_progress_callback cb_;
};

}    // namespace leaf

#endif
