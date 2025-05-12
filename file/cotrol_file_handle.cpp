#include <stack>
#include <string>
#include <filesystem>

#include "log/log.h"
#include "file/file.h"
#include "crypt/easy.h"
#include "config/config.h"
#include "protocol/codec.h"
#include "file/cotrol_file_handle.h"

namespace leaf
{
cotrol_file_handle ::cotrol_file_handle(std::string id, leaf::websocket_session::ptr& session)
    : id_(std::move(id)), session_(session)
{
    LOG_INFO("create {}", id_);
}

cotrol_file_handle::~cotrol_file_handle() { LOG_INFO("destroy {}", id_); }

void cotrol_file_handle ::startup()
{
    LOG_INFO("startup {}", id_);
    // clang-format off
    session_->set_read_cb([this, self = shared_from_this()](boost::beast::error_code ec, const std::vector<uint8_t>& bytes) { on_read(ec, bytes); });
    session_->set_write_cb([this, self = shared_from_this()](boost::beast::error_code ec, std::size_t bytes_transferred) { on_write(ec, bytes_transferred); });
    session_->startup();
    // clang-format on
}

void cotrol_file_handle ::shutdown()
{
    std::call_once(shutdown_flag_, [this, self = shared_from_this()]() { safe_shutdown(); });
}

void cotrol_file_handle::safe_shutdown()
{
    session_->shutdown();
    session_.reset();
    LOG_INFO("shutdown {}", id_);
}

void cotrol_file_handle ::on_read(boost::beast::error_code ec, const std::vector<uint8_t>& bytes)
{
    if (ec)
    {
        shutdown();
        return;
    }
    if (bytes.empty())
    {
        return;
    }
    auto type = get_message_type(bytes);
    if (type == leaf::message_type::files_request)
    {
        on_files_request(leaf::deserialize_files_request(bytes));
    }
}

void cotrol_file_handle::on_write(boost::beast::error_code ec, std::size_t /*transferred*/)
{
    if (ec)
    {
        shutdown();
        return;
    }
}
static std::vector<leaf::file_node> lookup_dir(const std::filesystem::path& dir)
{
    if (!std::filesystem::exists(dir))
    {
        return {};
    }
    std::stack<std::string> dirs;
    dirs.push(dir);
    std::vector<leaf::file_node> files;
    while (!dirs.empty())
    {
        std::string path = dirs.top();
        dirs.pop();
        for (const auto& entry : std::filesystem::directory_iterator(path))
        {
            std::string filename = entry.path().string();
            leaf::file_node f;
            f.name = filename;
            f.parent = path;
            f.type = "file";
            if (entry.is_directory())
            {
                f.type = "dir";
                dirs.push(entry.path().string());
            }
            else if (entry.is_regular_file())
            {
                std::string ext = entry.path().extension();
                if (ext != kLeafFilenameSuffix)
                {
                    continue;
                }
            }
            files.push_back(f);
        }
    }
    return files;
}

static std::vector<leaf::file_node> lookup_dir(const std::string& dir)
{
    return lookup_dir(std::filesystem::path(dir));
}

void cotrol_file_handle::on_files_request(const std::optional<leaf::files_request>& message)
{
    if (!message.has_value())
    {
        return;
    }
    const auto& msg = message.value();

    std::filesystem::path root_path(leaf::make_file_path(msg.token));
    auto dir_path = root_path.append(msg.dir);

    leaf::files_response response;
    // 递归遍历目录中的所有文件
    auto files = lookup_dir(dir_path);
    for (auto&& file : files)
    {
        file.name = leaf::decode(leaf::decode_leaf_filename(file.name));
    }
    response.token = msg.token;
    response.files.swap(files);
    LOG_INFO("{} on_files_request dir {}", id_, dir_path.filename().string());
    session_->write(leaf::serialize_files_response(response));
}
}    // namespace leaf
