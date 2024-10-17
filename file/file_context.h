#ifndef LEAF_TASK_ITEM_H
#define LEAF_TASK_ITEM_H

#include <string>
#include <chrono>
#include <memory>
#include <boost/asio.hpp>
#include "file.h"

namespace leaf
{
struct file_context
{
    using ptr = std::shared_ptr<file_context>;
    int32_t progress = 0;
    uint32_t block_size = 0;

    uint64_t id = 0;
    uint64_t file_size = 0;
    uint64_t block_count = 0;
    uint64_t active_block_count = 0;
    std::string name;
    std::string dst_file;
    std::string src_hash;
    std::string dst_hash;
    boost::system::error_code ec;
    std::chrono::time_point<std::chrono::steady_clock> start_time_;
};

}    // namespace leaf

#endif
