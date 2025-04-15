#include <stack>
#include <string>
#include <filesystem>

#include "log/log.h"
#include "file/file.h"
#include "protocol/codec.h"
#include "file/cotrol_file_handle.h"

namespace leaf
{
cotrol_file_handle ::cotrol_file_handle(std::string id) : id_(std::move(id)) { LOG_INFO("create {}", id_); }

cotrol_file_handle::~cotrol_file_handle() { LOG_INFO("destroy {}", id_); }

void cotrol_file_handle ::startup() { LOG_INFO("startup {}", id_); }

void cotrol_file_handle ::shutdown() { LOG_INFO("shutdown {}", id_); }

void cotrol_file_handle ::on_message(const leaf::websocket_session::ptr& session,
                                     const std::shared_ptr<std::vector<uint8_t>>& bytes)
{
    if (bytes == nullptr)
    {
        return;
    }
    if (bytes->empty())
    {
        return;
    }
    auto type = get_message_type(*bytes);
    if (type == leaf::message_type::files_request)
    {
        on_files_request(leaf::deserialize_files_request(*bytes));
    }

    while (!msg_queue_.empty())
    {
        session->write(msg_queue_.front());
        msg_queue_.pop();
    }
}
static std::vector<leaf::files_response::file_node> lookup_dir(const std::string& dir)
{
    if (!std::filesystem::exists(dir))
    {
        return {};
    }
    std::stack<std::string> dirs;
    dirs.push(dir);
    std::vector<leaf::files_response::file_node> files;
    while (!dirs.empty())
    {
        std::string path = dirs.top();
        dirs.pop();
        for (const auto& entry : std::filesystem::directory_iterator(path))
        {
            leaf::files_response::file_node f;
            f.name = entry.path().string();
            f.parent = path;
            f.type = "file";
            if (entry.is_directory())
            {
                f.type = "dir";
                dirs.push(entry.path().string());
            }
            files.push_back(f);
        }
    }
    return files;
}

void cotrol_file_handle::on_files_request(const std::optional<leaf::files_request>& message)
{
    if (!message.has_value())
    {
        return;
    }
    const auto& msg = message.value();

    std::string dir = leaf::make_file_path(msg.token);
    leaf::files_response response;
    // 递归遍历目录中的所有文件
    auto files = lookup_dir(dir);
    response.token = msg.token;
    response.files.swap(files);
    LOG_INFO("{} on_files_request dir {}", id_, dir);
    msg_queue_.push(leaf::serialize_files_response(response));
}
}    // namespace leaf
