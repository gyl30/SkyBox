#include "file/file_session_manager.h"

namespace leaf
{
void file_session_manager::add_session(const std::string &id, const std::shared_ptr<file_session> &session)
{
    std::lock_guard<std::mutex> lock(mutex_);
    sessions_[id] = session;
}

std::shared_ptr<file_session> file_session_manager::get_session(const std::string &id)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (sessions_.find(id) == sessions_.end())
    {
        return nullptr;
    }
    return sessions_[id];
}

}    // namespace leaf
