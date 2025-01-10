#ifndef LEAF_FILE_EVENT_H
#define LEAF_FILE_EVENT_H

#include <string>
#include <functional>

namespace leaf
{

struct upload_event
{
    uint64_t id;
    uint64_t file_size;
    uint64_t upload_size;
    std::string filename;
};
struct notify_event
{

};
struct download_event
{
    uint64_t id;
    uint64_t file_size;
    uint64_t download_size;
    std::string filename;
};

using download_progress_callback = std::function<void(const download_event&)>;
using upload_progress_callback = std::function<void(const upload_event&)>;
using notify_progress_callback = std::function<void(const notify_event&)>;

}    // namespace leaf

#endif
