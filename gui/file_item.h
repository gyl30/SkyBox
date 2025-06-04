#ifndef LEAF_FILE_ITEM_H
#define LEAF_FILE_ITEM_H

#include <memory>
#include <string>
#include <vector>

enum class file_item_type : uint8_t
{
    Folder,
    File
};

struct file_item
{
    file_item_type type;
    int64_t file_size = 0;
    uint64_t last_modified;
    std::string display_name;
    std::string storage_name;
    std::weak_ptr<file_item> parent;
    std::vector<std::shared_ptr<file_item>> children;
};

#endif
