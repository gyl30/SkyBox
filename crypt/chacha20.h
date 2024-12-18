#ifndef LEAF_CRYPT_CHACHA20_H
#define LEAF_CRYPT_CHACHA20_H

#include <cstdint>
#include <vector>
#include "crypt/crypt.h"

namespace leaf
{
class chacha20_encrypt : public leaf::encrypt
{
   public:
    explicit chacha20_encrypt(std::vector<uint8_t> key);
    ~chacha20_encrypt() override;

   public:
    std::vector<uint8_t> encode(const std::vector<uint8_t>& plaintext, boost::system::error_code& ec) override;
    std::size_t padding() override;
    std::size_t prefix() override;

   private:
    class chacha20_encrypt_impl;
    chacha20_encrypt_impl* impl_;
};
class chacha20_decrypt : public leaf::decrypt
{
   public:
    explicit chacha20_decrypt(std::vector<uint8_t> key);
    ~chacha20_decrypt() override;

   public:
    std::vector<uint8_t> decode(const std::vector<uint8_t>& ciphertext, boost::system::error_code& ec) override;
    std::size_t padding() override;
    std::size_t prefix() override;

   private:
    class chacha20_decrypt_impl;
    chacha20_decrypt_impl* impl_;
};

}    // namespace leaf

#endif
