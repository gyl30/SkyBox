#ifndef LEAF_FILE_FILE_CONTEXT_H
#define LEAF_FILE_FILE_CONTEXT_H

#include <string>
#include <memory>
#include <boost/asio.hpp>

namespace leaf
{
struct file_context
{
    using ptr = std::shared_ptr<file_context>;

    uint64_t id = 0;
    uint64_t file_size = 0;
    uint32_t block_size = 0;
    uint32_t padding_size = 0;
    uint32_t hash_block_count = 0;
    uint64_t block_count = 0;
    uint64_t active_block_count = 0;
    std::string file_path;
    std::string filename;
    std::string content_hash;
    std::string filename_hash;
};

}    // namespace leaf

#endif
