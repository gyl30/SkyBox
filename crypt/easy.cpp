#include <sodium.h>
#include <boost/algorithm/hex.hpp>

#include "crypt/easy.h"

namespace leaf
{

static unsigned char nonce[crypto_secretbox_NONCEBYTES] = {0};

static unsigned char key[crypto_secretbox_KEYBYTES] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b,
                                                       0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16,
                                                       0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20};

static std::string encode(const uint8_t* plain, uint32_t plain_len)
{
    std::vector<uint8_t> ciphertext(plain_len + crypto_secretbox_MACBYTES, 0);
    if (crypto_secretbox_easy(ciphertext.data(), plain, plain_len, nonce, key) != 0)
    {
        return {};
    }
    std::string hex_str;
    boost::algorithm::hex_lower(ciphertext.begin(), ciphertext.end(), std::back_inserter(hex_str));
    return hex_str;
}

static std::string decode(const uint8_t* ciphertext, uint32_t ciphertext_len)
{
    std::vector<uint8_t> plain(ciphertext_len - crypto_secretbox_MACBYTES, 0);
    if (crypto_secretbox_open_easy(plain.data(), ciphertext, ciphertext_len, nonce, key) != 0)
    {
        return {};
    }
    std::string hex_str;
    boost::algorithm::hex_lower(plain.begin(), plain.end(), std::back_inserter(hex_str));
    return hex_str;
}

std::string encode(const std::vector<uint8_t>& text) { return encode(text.data(), text.size()); }

std::string encode(const std::string& text)
{
    return encode(reinterpret_cast<const uint8_t*>(text.data()), text.size());
}

std::string decode(const std::vector<uint8_t>& ciphertext) { return decode(ciphertext.data(), ciphertext.size()); }

std::string decode(const std::string& ciphertext)
{
    return decode(reinterpret_cast<const uint8_t*>(ciphertext.data()), ciphertext.size());
}
}    // namespace leaf
