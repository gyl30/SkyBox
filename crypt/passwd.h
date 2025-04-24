#ifndef LEAF_CRYPT_CRYPT_PASSWD_H
#define LEAF_CRYPT_CRYPT_PASSWD_H

#include <string>
#include <vector>
#include <cstdint>

namespace leaf
{
std::vector<uint8_t> passwd_key();
std::string passwd_hash(const std::string& passwd, const std::vector<uint8_t>& key);
std::string passwd_hash(const std::string& passwd, const std::string& key);
}    // namespace leaf
#endif
