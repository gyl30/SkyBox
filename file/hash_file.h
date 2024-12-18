#ifndef LEAF_FILE_HASH_FILE_H
#define LEAF_FILE_HASH_FILE_H

#include <string>
#include <boost/system/error_code.hpp>

namespace leaf
{
std::string hash_file(const std::string& file, boost::system::error_code& ec);
}

#endif
