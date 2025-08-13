#ifndef LEAF_NET_EXCEPTION_H
#define LEAF_NET_EXCEPTION_H

#include <string>
#include <exception>

namespace leaf
{

void cache_exception(const std::string& msg, const std::exception_ptr& eptr);
}

#endif
