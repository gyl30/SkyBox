#include <vector>
#include <string>
#include <sodium.h>
#include <boost/algorithm/hex.hpp>

#include "blake2b.h"

namespace leaf
{
class blake2b::blake2b_impl
{
   public:
    blake2b_impl() { crypto_generichash_init(&state_, nullptr, 0, crypto_generichash_BYTES_MAX); }
    ~blake2b_impl() = default;

   public:
    std::string hex()
    {
        std::string hex_str;
        boost::algorithm::hex_lower(bytes_, bytes_ + crypto_generichash_BYTES_MAX, std::back_inserter(hex_str));
        return hex_str;
    }

    std::vector<uint8_t> bytes() { return std::vector<uint8_t>{bytes_, bytes_ + crypto_generichash_BYTES_MAX}; }

    void update(const void* buffer, uint32_t buffer_len)
    {
        if (buffer == nullptr || buffer_len == 0)
        {
            return;
        }
        crypto_generichash_update(&state_, static_cast<const uint8_t*>(buffer), buffer_len);
    }

    void final() { crypto_generichash_final(&state_, bytes_, crypto_generichash_BYTES_MAX); }

   private:
    crypto_generichash_state state_;
    unsigned char bytes_[crypto_generichash_BYTES_MAX] = {0};
};

blake2b::blake2b() { impl = new blake2b_impl(); }
blake2b::~blake2b() { delete impl; }

std::string blake2b::hex() { return impl->hex(); }

void blake2b::reset()
{
    delete impl;
    impl = new blake2b_impl();
}
std::vector<uint8_t> blake2b::bytes() { return impl->bytes(); }

void blake2b::update(const void* buffer, uint32_t buffer_len) { impl->update(buffer, buffer_len); }

void blake2b::final() { impl->final(); }
}    // namespace leaf
