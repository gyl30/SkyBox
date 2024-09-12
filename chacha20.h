#ifndef LEAF_CHACHA20_H
#define LEAF_CHACHA20_H

#include <cstdint>
#include <vector>

namespace leaf
{
class chacha20_encrypt
{
   public:
    explicit chacha20_encrypt(std::vector<uint8_t> key);
    ~chacha20_encrypt();

   public:
    std::vector<uint8_t> encode(const std::vector<uint8_t>& plaintext);

   private:
    class chacha20_encrypt_impl;
    chacha20_encrypt_impl* impl_;
};
class chacha20_decrypt
{
   public:
    explicit chacha20_decrypt(std::vector<uint8_t> key);
    ~chacha20_decrypt();

   public:
    std::vector<uint8_t> decode(const std::vector<uint8_t>& ciphertext);

   private:
    class chacha20_decrypt_impl;
    chacha20_decrypt_impl* impl_;
};

}    // namespace leaf

#endif
