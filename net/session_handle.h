#ifndef LEAF_HANDLE_H
#define LEAF_HANDLE_H

#include "http_handle.h"
#include "websocket_handle.h"

namespace leaf
{

struct session_handle
{
    std::function<leaf::http_handle::ptr(void)> http_handle;
    std::function<leaf::websocket_handle::ptr(void)> websocket_handle;
};

}    // namespace leaf

#endif
