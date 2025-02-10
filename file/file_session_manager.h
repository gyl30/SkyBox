#ifndef LEAF_FILE_FILE_SESSION_MANAGER_H
#define LEAF_FILE_FILE_SESSION_MANAGER_H

#include <map>
#include <memory>
#include <string>
#include "util/singleton.h"
#include "file/file_session.h"

namespace leaf
{

class file_session_manager
{
   public:
    file_session_manager() = default;
    ~file_session_manager() = default;

   public:
    void add_session(const std::string &id, const std::shared_ptr<file_session> &session);
    std::shared_ptr<file_session> get_session(const std::string &id);

   private:
    std::map<std::string, std::shared_ptr<file_session>> sessions_;
};

using fsm = singleton<file_session_manager>;
}    // namespace leaf

#endif
