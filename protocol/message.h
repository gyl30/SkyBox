#ifndef LEAF_PROTOCOL_MESSAGE_H
#define LEAF_PROTOCOL_MESSAGE_H

#include <string>
#include <vector>

namespace leaf
{
enum class message_type : uint16_t
{
    error = 0x00,
    upload_file_request = 0x01,
    upload_file_response = 0x02,
    delete_file_request = 0x03,
    delete_file_response = 0x04,
    file_block_request = 0x05,
    file_block_response = 0x06,
    block_data_request = 0x07,
    block_data_response = 0x08,
    block_data_finish = 0x09,
    upload_file_exist = 0x10,
    download_file_request = 0x11,
    download_file_response = 0x12,
    keepalive = 0x13,
    login_request = 0x14,
    login_response = 0x15,
    files_request = 0x16,
    files_response = 0x17
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
};

struct login_response
{
    std::string username;
    std::string token;
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
