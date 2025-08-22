#ifndef LEAF_FILE_HASH_FILE_H
#define LEAF_FILE_HASH_FILE_H

#include <limits>
#include <string>
#include <boost/system/error_code.hpp>

namespace leaf
{
std::string hash_file(const std::string& file, boost::system::error_code& ec, std::size_t read_limit = std::numeric_limits<std::size_t>::max());
}

#endif
