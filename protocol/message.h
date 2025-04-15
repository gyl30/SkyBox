#ifndef LEAF_PROTOCOL_MESSAGE_H
#define LEAF_PROTOCOL_MESSAGE_H

#include <string>
#include <vector>

namespace leaf
{
enum class message_type : uint16_t
{
    error = 0x00,
    login = 0x01,
    upload_file_request = 2,
    delete_file_request = 4,
    delete_file_response = 5,
    download_file_request = 6,
    download_file_response = 7,
    keepalive = 8,
    files_request = 9,
    files_response = 10,
    file_data,
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
};

struct files_response
{
    uint32_t id = 0;
    struct file_node
    {
        std::string parent;
        std::string name;
        std::string type;
    };
    std::string token;
    std::vector<file_node> files;
};
// ------------------------------------------------------------------------------
struct upload_file_request
{
    uint32_t id = 0;
    uint32_t block_count = 0;
    uint32_t padding_size = 0;
    std::string filename;
};
struct file_data
{
    uint32_t block_id = 0;
    std::string hash;
    std::vector<uint8_t> data;
};
struct error_message
{
    uint32_t id = 0;
    uint32_t error = 0;
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
    std::string filename;    // 文件名称
};

struct download_file_response
{
    uint32_t id = 0;
    uint32_t block_count = 0;
    uint32_t padding_size = 0;
    std::string filename;    // 文件名称
};

struct delete_file_request
{
    uint32_t id = 0;
    std::string filename;    // 文件名称
};

}    // namespace leaf

#endif
