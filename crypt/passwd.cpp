#include <sodium.h>
#include <cstring>
#include <vector>
#include <boost/algorithm/hex.hpp>
#include "crypt/passwd.h"

namespace leaf
{
std::vector<uint8_t> passwd_key()
{
    std::vector<uint8_t> result;
    unsigned char key[crypto_generichash_KEYBYTES];
    randombytes_buf(key, sizeof key);
    result.assign(key, key + sizeof key);
    return result;
}
std::string passwd_hash(const std::string& passwd, const std::vector<uint8_t>& key)
{
    unsigned char hash[crypto_generichash_BYTES];
    int ret = crypto_generichash(
        hash, sizeof hash, reinterpret_cast<const uint8_t*>(passwd.data()), passwd.size(), key.data(), key.size());
    if (ret != 0)
    {
        return {};
    }
    std::string hex_str;
    boost::algorithm::hex_lower(hash, hash + sizeof hash, std::back_inserter(hex_str));
    return hex_str;
}
}    // namespace leaf
