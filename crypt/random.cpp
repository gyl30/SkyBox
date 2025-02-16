#include <sodium.h>
#include <boost/algorithm/hex.hpp>

#include "crypt/random.h"

namespace leaf
{

uint32_t random_uint32() { return randombytes_random(); }

std::string random_string(size_t len)
{
    auto buf = random_bytes(len);
    std::string hex_str;
    boost::algorithm::hex_lower(buf, std::back_inserter(hex_str));
    return hex_str;
}
std::vector<uint8_t> random_bytes(size_t len)
{
    std::vector<uint8_t> result;
    auto *buf = new uint8_t[len];
    randombytes_buf(buf, len);
    result.assign(buf, buf + len);
    delete[] buf;
    return result;
}

}    // namespace leaf
