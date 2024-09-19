#ifndef LEAF_TASK_ITEM_H
#define LEAF_TASK_ITEM_H

#include <string>
#include <chrono>
#include <boost/asio.hpp>

namespace leaf
{
struct task_item
{
    int progress = 0;
    std::string src_file;
    std::string dst_file;
    std::string src_hash;
    std::string dst_hash;
    boost::system::error_code ec;
    std::chrono::time_point<std::chrono::steady_clock> start_time_;
};

}    // namespace leaf

#endif
