#ifndef LEAF_FILE_EVENT_H
#define LEAF_FILE_EVENT_H

#include <any>
#include <string>
#include <functional>

namespace leaf
{

struct upload_event
{
    uint64_t file_size;
    uint64_t upload_size;
    std::string filename;
};
struct notify_event
{
    std::string method;
    std::any data;
};
struct download_event
{
    uint64_t file_size;
    uint64_t download_size;
    std::string filename;
};

struct cotrol_event
{
    std::string token;
};

using download_progress_callback = std::function<void(const download_event&)>;
using upload_progress_callback = std::function<void(const upload_event&)>;
using notify_progress_callback = std::function<void(const notify_event&)>;
using cotrol_progress_callback = std::function<void(const cotrol_event&)>;

struct progress_handler
{
    download_progress_callback download;
    upload_progress_callback upload;
    notify_progress_callback notify;
    cotrol_progress_callback cotrol;
};

}    // namespace leaf

#endif
