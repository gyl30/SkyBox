#ifndef LEAF_HANDLE_H
#define LEAF_HANDLE_H

#include "http_handle.h"

namespace leaf
{

struct session_handle
{
    std::function<leaf::http_handle::ptr(void)> http_handle;
};

}    // namespace leaf

#endif
