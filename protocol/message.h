#ifndef LEAF_PROTOCOL_MESSAGE_H
#define LEAF_PROTOCOL_MESSAGE_H

#include <string>
#include <vector>

namespace leaf
{
enum class message_type : uint16_t
{
    error = 0x00,
    upload_file_request = 1,
    upload_file_response = 2,
    delete_file_request = 3,
    delete_file_response = 4,
    file_block_request = 5,
    file_block_response = 6,
    block_data_request = 7,
    block_data_response = 8,
    block_data_finish = 9,
    upload_file_exist = 10,
    download_file_request = 11,
    download_file_response = 12,
    keepalive = 13,
    login_request = 14,
    login_response = 15,
    files_request = 16,
    files_response = 17,
    login_token = 18,
};

struct login_token
{
    std::string token;
    uint32_t block_size = 0;
};

struct files_request
{
    std::string token;
};

struct files_response
{
    struct file_node
    {
        std::string parent;
        std::string name;
        std::string type;
        std::vector<file_node> children;
    };
    std::string token;
    std::vector<file_node> files;
};
// ------------------------------------------------------------------------------
struct upload_file_request
{
    uint64_t file_size = 0;    // 文件大小
    std::string hash;          // 文件 hash
    std::string filename;      // 文件名称
};
struct upload_file_response
{
    uint64_t file_id = 0;       // 文件唯一标识
    uint32_t block_size = 0;    // 文件块大小
    std::string filename;       // 文件名称
};

struct login_request
{
    std::string username;
    std::string password;
    std::string token;
};

struct login_response
{
    std::string username;
    std::string token;
};

struct create_dir_request
{
    std::string token;
    std::string parent;
    std::string dir;
};

struct create_dir_response
{
    std::string token;
    std::string parent;
};

struct keepalive
{
    uint64_t id = 0;                  // 消息 id
    uint64_t client_id = 0;           // 客户端 id
    uint64_t client_timestamp = 0;    // 客户端时间
    uint64_t server_timestamp = 0;    // 服务端时间
    std::string token;
};

struct download_file_request
{
    std::string filename;    // 文件名称
};

struct download_file_response
{
    uint64_t file_id = 0;      // 文件唯一标识
    uint64_t file_size = 0;    // 文件大小
    std::string hash;          // 文件 hash
    std::string filename;      // 文件名称
};

struct file_block_request
{
    uint64_t file_id = 0;    // 文件唯一标识
};
struct file_block_response
{
    uint64_t file_id = 0;        // 文件唯一标识
    uint32_t block_count = 0;    // 文件块数量
    uint32_t block_size = 0;     // 文件块大小
};

struct delete_file_request
{
    std::string filename;    // 文件名称
};

struct delete_file_response
{
    std::string filename;    // 文件名称
};

struct block_data_request
{
    uint32_t block_id = 0;    // 块 id
    uint64_t file_id = 0;     // 文件 id
};

struct block_data_response
{
    uint64_t file_id = 0;         // 文件 id
    uint32_t block_id = 0;        // 块 id
    std::vector<uint8_t> data;    // 块数据
};

struct block_data_finish
{
    uint64_t file_id = 0;    // 文件 id
    std::string hash;        // 文件 hash
    std::string filename;    // 文件名称
};

struct upload_file_exist
{
    std::string hash;        // 文件 hash
    std::string filename;    // 文件名称
};
struct error_response
{
    uint32_t error = 0;
    std::string message;
};
}    // namespace leaf

#endif
