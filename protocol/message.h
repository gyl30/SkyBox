#ifndef LEAF_MESSAGE_H
#define LEAF_MESSAGE_H

#include <string>
#include <vector>

namespace leaf
{
enum class message_type : uint16_t
{
    error = 0x00,
    create_file_request = 0x01,
    create_file_response = 0x02,
    delete_file_request = 0x02,
    delete_file_response = 0x03,
    file_block_request = 0x04,
    file_block_response = 0x05,
    block_data_request = 0x06,
    block_data_response = 0x07,
};

// ------------------------------------------------------------------------------
// request 4 bytes length + 2 bytes type + payload
struct create_file_request
{
    uint64_t file_size = 0;
    std::string filename;
};
struct file_block_request
{
    uint64_t file_id = 0;
};
struct delete_file_request
{
    std::string filename;
};
struct block_data_request
{
    // block data request payload(4 bytes block id + random data) == 128k
    uint32_t block_id = 0;
    uint64_t file_id = 0;
    std::vector<uint8_t> data;
};
//
// ------------------------------------------------------------------------------
struct error_response
{
    uint32_t error = 0;
};
// response 4 bytes length + 2 bytes id
struct create_file_response
{
    uint64_t file_id = 0;
    std::string filename;
};
struct file_block_response
{
    uint32_t block_count = 0;
    uint32_t block_size = 0;
};
struct delete_file_response
{
    std::string filename;
};
struct block_data_response
{
    // block data response payload(4 bytes block id + data)
    uint64_t file_id = 0;
    uint32_t block_id = 0;
    std::vector<uint8_t> data;
};
}    // namespace leaf

#endif
