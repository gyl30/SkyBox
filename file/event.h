#ifndef LEAF_FILE_EVENT_H
#define LEAF_FILE_EVENT_H

#include <any>
#include <string>
#include <functional>
#include <boost/asio.hpp>

namespace leaf
{

struct upload_event
{
    uint64_t file_size = 0;
    uint64_t upload_size = 0;
    std::string filename;
};
struct notify_event
{
    std::string method;
    std::any data;
};
struct download_event
{
    uint64_t file_size = 0;
    uint64_t download_size = 0;
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
using error_progress_callback = std::function<void(const boost::system::error_code&)>;

struct download_handle
{
    download_progress_callback download;
    error_progress_callback error;
    notify_progress_callback notify;
};
struct upload_handle
{
    upload_progress_callback upload;
    error_progress_callback error;
    notify_progress_callback notify;
};

struct cotrol_handle
{
    cotrol_progress_callback cotrol;
    error_progress_callback error;
    notify_progress_callback notify;
};

struct progress_handler
{
    cotrol_handle c;
    upload_handle u;
    download_handle d;
};

}    // namespace leaf

#endif
