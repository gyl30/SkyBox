#ifndef LEAF_GUI_TASK_H
#define LEAF_GUI_TASK_H

#include <string>
#include <cstdint>

namespace leaf
{

struct task
{
    std::string op;
    uint64_t file_size;
    uint64_t process_size;
    std::string filename;
};

}    // namespace leaf

#endif
