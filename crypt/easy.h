#ifndef LEAF_CRYPT_EASY_H
#define LEAF_CRYPT_EASY_H

#include <string>
#include <cstdint>
#include <vector>

namespace leaf
{
std::string encode(const std::string& text);
std::string encode(const std::vector<uint8_t>& text);
std::string decode(const std::string& ciphertext);
std::string decode(const std::vector<uint8_t>& ciphertext);
}    // namespace leaf

#endif
