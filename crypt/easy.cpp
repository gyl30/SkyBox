#include <sodium.h>
#include <boost/algorithm/hex.hpp>

#include "crypt/easy.h"

namespace leaf
{

static unsigned char nonce[crypto_secretbox_NONCEBYTES] = {0};

static unsigned char key[crypto_secretbox_KEYBYTES] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b,
                                                       0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16,
                                                       0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20};

static std::string encode(const uint8_t* plaintext, uint32_t plaintext_len)
{
    size_t ciphertext_len = crypto_secretbox_MACBYTES + plaintext_len;
    std::vector<unsigned char> ciphertext(ciphertext_len);
    if (crypto_secretbox_easy(ciphertext.data(), plaintext, plaintext_len, nonce, key) != 0)
    {
        return {};
    }
    std::string hex_str;
    boost::algorithm::hex(ciphertext.begin(), ciphertext.end(), std::back_inserter(hex_str));
    return hex_str;
}

static std::string decode(const uint8_t* hex_ciphertext, uint32_t hex_ciphertext_len)
{
    std::vector<unsigned char> ciphertext;
    boost::algorithm::unhex(hex_ciphertext, hex_ciphertext + hex_ciphertext_len, std::back_inserter(ciphertext));
    size_t plaintext_len = ciphertext.size() - crypto_secretbox_MACBYTES;
    std::vector<unsigned char> decrypted(plaintext_len);
    if (crypto_secretbox_open_easy(decrypted.data(), ciphertext.data(), ciphertext.size(), nonce, key) != 0)
    {
        return {};
    }
    std::string str(decrypted.begin(), decrypted.end());
    return str;
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
