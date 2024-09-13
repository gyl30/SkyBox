#include <cassert>
#include <sodium.h>
#include "chacha20.h"

namespace leaf
{
class chacha20_encrypt::chacha20_encrypt_impl
{
   public:
    explicit chacha20_encrypt_impl(std::vector<uint8_t> key)
    {
        assert(key.size() == crypto_secretstream_xchacha20poly1305_KEYBYTES);
        for (auto i = 0U; i < crypto_secretstream_xchacha20poly1305_KEYBYTES; i++)
        {
            key_[i] = key[i];
        }
    };

   public:
    std::vector<uint8_t> encode(const std::vector<uint8_t>& plaintext, boost::system::error_code& ec)
    {
        std::vector<uint8_t> result;
        if (first_)
        {
            unsigned char header[crypto_secretstream_xchacha20poly1305_HEADERBYTES];
            int ret = crypto_secretstream_xchacha20poly1305_init_push(&st, header, key_);
            if (ret != 0)
            {
                static constexpr boost::source_location loc = BOOST_CURRENT_LOCATION;
                ec.assign(ret, boost::system::generic_category(), &loc);
                return {};
            }
            result.insert(result.end(), header, header + crypto_secretstream_xchacha20poly1305_HEADERBYTES);
            first_ = false;
        }

        std::vector<uint8_t> ciphertext(plaintext.size() + crypto_secretstream_xchacha20poly1305_ABYTES, '0');
        unsigned long long outlen = 0;    // NOLINT
        int tag = plaintext.empty() ? crypto_secretstream_xchacha20poly1305_TAG_FINAL : 0;
        int ret = crypto_secretstream_xchacha20poly1305_push(&st, static_cast<uint8_t*>(ciphertext.data()), &outlen, plaintext.data(), plaintext.size(), nullptr, 0, tag);
        if (ret != 0)
        {
            static constexpr boost::source_location loc = BOOST_CURRENT_LOCATION;
            ec.assign(ret, boost::system::generic_category(), &loc);
            return {};
        }
        result.insert(result.end(), ciphertext.begin(), ciphertext.begin() + static_cast<int64_t>(outlen));
        return result;
    }

   private:
    bool first_ = true;
    crypto_secretstream_xchacha20poly1305_state st;
    unsigned char key_[crypto_secretstream_xchacha20poly1305_KEYBYTES] = {0};
};
class chacha20_decrypt::chacha20_decrypt_impl
{
   public:
    explicit chacha20_decrypt_impl(std::vector<uint8_t> key)
    {
        assert(key.size() == crypto_secretstream_xchacha20poly1305_KEYBYTES);
        for (auto i = 0U; i < crypto_secretstream_xchacha20poly1305_KEYBYTES; i++)
        {
            key_[i] = key[i];
        }
    };

   public:
    std::vector<uint8_t> decode(const std::vector<uint8_t>& ciphertext, boost::system::error_code& ec)
    {
        std::vector<uint8_t> result;
        if (first_)
        {
            return decode_header(ciphertext, ec);
        }
        return decode_payload(ciphertext, ec);
    }

   private:
    std::vector<uint8_t> decode_payload(const std::vector<uint8_t>& ciphertext, boost::system::error_code& ec)
    {
        std::vector<uint8_t> plaintext(ciphertext.size(), '0');
        unsigned long long out_len = 0;    // NOLINT
        unsigned char tag;
        int ret = crypto_secretstream_xchacha20poly1305_pull(&st, static_cast<uint8_t*>(plaintext.data()), &out_len, &tag, ciphertext.data(), ciphertext.size(), nullptr, 0);
        if (ret != 0)
        {
            static constexpr boost::source_location loc = BOOST_CURRENT_LOCATION;
            ec.assign(ret, boost::system::generic_category(), &loc);
            return {};
        }
        plaintext.resize(static_cast<size_t>(out_len));
        return plaintext;
    }
    std::vector<uint8_t> decode_header(const std::vector<uint8_t>& ciphertext, boost::system::error_code& ec)
    {
        header_.insert(header_.end(), ciphertext.begin(), ciphertext.end());
        if (header_.size() < crypto_secretstream_xchacha20poly1305_HEADERBYTES)
        {
            return {};
        }
        unsigned char header[crypto_secretstream_xchacha20poly1305_HEADERBYTES];
        for (auto i = 0U; i < crypto_secretstream_xchacha20poly1305_HEADERBYTES; i++)
        {
            header[i] = header_[i];
        }
        int ret = crypto_secretstream_xchacha20poly1305_init_pull(&st, header, key_);
        if (ret != 0)
        {
            static constexpr boost::source_location loc = BOOST_CURRENT_LOCATION;
            ec.assign(ret, boost::system::generic_category(), &loc);
            return {};
        }
        first_ = false;
        if (header_.size() == crypto_secretstream_xchacha20poly1305_HEADERBYTES)
        {
            return {};
        }
        header_.erase(header_.begin(), header_.begin() + crypto_secretstream_xchacha20poly1305_HEADERBYTES);
        return decode_payload(header_, ec);
    }

   private:
    bool first_ = true;
    std::vector<uint8_t> header_;
    crypto_secretstream_xchacha20poly1305_state st;
    unsigned char key_[crypto_secretstream_xchacha20poly1305_KEYBYTES] = {0};
};
chacha20_encrypt::chacha20_encrypt(std::vector<uint8_t> key) : impl_(new chacha20_encrypt_impl(std::move(key))) {}

chacha20_encrypt::~chacha20_encrypt() { delete impl_; }

std::vector<uint8_t> chacha20_encrypt::encode(const std::vector<uint8_t>& plaintext, boost::system::error_code& ec) { return impl_->encode(plaintext, ec); }

chacha20_decrypt::chacha20_decrypt(std::vector<uint8_t> key) : impl_(new chacha20_decrypt_impl(std::move(key))) {}
chacha20_decrypt::~chacha20_decrypt() { delete impl_; }
std::vector<uint8_t> chacha20_decrypt::decode(const std::vector<uint8_t>& ciphertext, boost::system::error_code& ec) { return impl_->decode(ciphertext, ec); }

}    // namespace leaf
