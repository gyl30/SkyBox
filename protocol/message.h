#ifndef LEAF_PROTOCOL_MESSAGE_H
#define LEAF_PROTOCOL_MESSAGE_H

#include <string>
#include <vector>
#include <cstdint>

namespace leaf
{
enum class message_type : uint8_t
{
    error = 0x00,
    login = 0x01,
    upload_file_request = 2,
    upload_file_response = 3,
    delete_file_request = 4,
    delete_file_response = 5,
    download_file_request = 6,
    download_file_response = 7,
    keepalive = 8,
    files_request = 9,
    files_response = 10,
    file_data = 11,
    ack = 12,
    done = 13,
    dir = 14,
};

struct create_dir
{
    std::string parent;
    std::string dir;
    std::string token;
};

struct login_request
{
    std::string username;
    std::string password;
};

struct login_token
{
    uint32_t id = 0;
    std::string token;
};

struct files_request
{
    std::string token;
    std::string dir;
};
struct file_node
{
    int64_t file_size = 0;
    std::string parent;
    std::string name;
    std::string type;
};

struct files_response
{
    uint32_t id = 0;
    std::string token;
    std::string dir;
    std::vector<file_node> files;
};
struct ack
{
};
struct done
{
};
// ------------------------------------------------------------------------------
struct upload_file_request
{
    uint32_t id = 0;
    uint64_t filesize = 0;
    std::string dir;
    std::string filename;
};
struct upload_file_response
{
    uint32_t id = 0;
    std::string filename;
};
struct file_data
{
    std::string hash;
    std::vector<uint8_t> data;
};
struct error_message
{
    uint32_t id = 0;
    int32_t error = 0;
};

struct keepalive
{
    uint64_t id = 0;                  // 消息 id
    uint64_t client_id = 0;           // 客户端 id
    uint64_t client_timestamp = 0;    // 客户端时间
    uint64_t server_timestamp = 0;    // 服务端时间
};

struct download_file_request
{
    uint32_t id = 0;
    std::string dir;
    std::string filename;    // 文件名称
};

struct download_file_response
{
    uint32_t id = 0;
    uint64_t filesize = 0;
    std::string filename;    // 文件名称
};

struct delete_file_request
{
    uint32_t id = 0;
    std::string filename;    // 文件名称
};

}    // namespace leaf

#endif
