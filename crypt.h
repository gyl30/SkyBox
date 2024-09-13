#ifndef LEAF_CRYPT_H
#define LEAF_CRYPT_H

#include <vector>
#include <boost/system/error_code.hpp>

namespace leaf
{
class encrypt
{
   public:
    virtual ~encrypt() = default;

   public:
    virtual std::vector<uint8_t> encode(const std::vector<uint8_t>& plaintext, boost::system::error_code& ec) = 0;
    virtual std::size_t padding() = 0;
    virtual std::size_t prefix() = 0;
};
class decrypt
{
   public:
    virtual ~decrypt() = default;

   public:
    virtual std::vector<uint8_t> decode(const std::vector<uint8_t>& ciphertext, boost::system::error_code& ec) = 0;
    virtual std::size_t padding() = 0;
    virtual std::size_t prefix() = 0;
};

}    // namespace leaf

#endif
