#ifndef LEAF_CRYPT_RANDOM_H
#define LEAF_CRYPT_RANDOM_H

#include <string>
#include <vector>

namespace leaf
{
uint32_t random_uint32();
std::string random_string(size_t len);
std::vector<uint8_t> random_bytes(size_t len);
}    // namespace leaf
#endif
