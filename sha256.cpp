#include <vector>
#include <string>
#include <sodium.h>
#include <boost/algorithm/hex.hpp>

#include "sha256.h"

namespace leaf
{
class sha256::sha256_impl
{
   public:
    sha256_impl() { crypto_hash_sha256_init(&state); }
    ~sha256_impl() = default;

   public:
    std::string hex()
    {
        std::string hex_str;
        boost::algorithm::hex_lower(sha256_, sha256_ + crypto_hash_sha256_BYTES, std::back_inserter(hex_str));
        return hex_str;
    }

    std::vector<uint8_t> bytes() { return std::vector<uint8_t>{sha256_, sha256_ + crypto_hash_sha256_BYTES}; }

    void update(const void* buffer, uint32_t buffer_len) { crypto_hash_sha256_update(&state, static_cast<const uint8_t*>(buffer), buffer_len); }

    void final() { crypto_hash_sha256_final(&state, sha256_); }

   private:
    crypto_hash_sha256_state state;
    unsigned char sha256_[crypto_hash_sha256_BYTES] = {0};
};

sha256::sha256() { impl = new sha256_impl(); }
sha256::~sha256() { delete impl; }

std::string sha256::hex() { return impl->hex(); }

std::vector<uint8_t> sha256::bytes() { return impl->bytes(); }

void sha256::update(const void* buffer, uint32_t buffer_len) { impl->update(buffer, buffer_len); }

void sha256::final() { impl->final(); }
}    // namespace leaf
