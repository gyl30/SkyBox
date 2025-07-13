#ifndef LEAF_FILE_FILE_ITEM_H
#define LEAF_FILE_FILE_ITEM_H

#include <memory>
#include <string>
#include <vector>

namespace leaf
{
enum class file_item_type : uint8_t
{
    Folder,
    File
};

struct file_item
{
    file_item_type type;
    int64_t file_size = 0;
    std::string display_name;
    std::string storage_name;
};

}    // namespace leaf
#endif
