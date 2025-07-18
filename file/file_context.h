#ifndef LEAF_FILE_FILE_CONTEXT_H
#define LEAF_FILE_FILE_CONTEXT_H

#include <string>
#include <boost/asio.hpp>

namespace leaf
{
struct file_info
{
    uint64_t file_size = 0;
    std::string filename;
    std::string local_path;
    std::string dir;
};

}    // namespace leaf

#endif
