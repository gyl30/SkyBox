#ifndef LEAF_FILE_FILE_CONTEXT_H
#define LEAF_FILE_FILE_CONTEXT_H

#include <string>
#include <memory>
#include <boost/asio.hpp>

namespace leaf
{
struct file_info
{
    using ptr = std::shared_ptr<file_info>;

    uint64_t file_size = 0;
    uint64_t hash_count = 0;
    int64_t offset = 0;
    std::string filename;
    std::string file_path;
};

}    // namespace leaf

#endif
