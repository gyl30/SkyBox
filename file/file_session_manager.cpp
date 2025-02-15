#include "crypt/passwd.h"
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

std::string file_session_manager::create_token()
{
    std::string id = leaf::passwd_hash("token", leaf::passwd_key());
    auto session = std::make_shared<file_session>();
    add_session(id, session);
    return id;
}

}    // namespace leaf
