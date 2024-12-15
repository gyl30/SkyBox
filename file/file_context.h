#ifndef LEAF_TASK_ITEM_H
#define LEAF_TASK_ITEM_H

#include <string>
#include <chrono>
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
    uint64_t block_count = 0;
    uint64_t active_block_count = 0;
    std::string file_path;
    std::string filename;
    std::string content_hash;
};

}    // namespace leaf

#endif
