#ifndef LEAF_SHA256_H
#define LEAF_SHA256_H

#include <vector>
#include <string>

namespace leaf
{
class sha256
{
   public:
    sha256();
    ~sha256();

   public:
    std::string hex();
    std::vector<uint8_t> bytes();
    void update(const void* buffer, uint32_t buffer_len);
    void final();

   private:
    class sha256_impl;
    sha256_impl* impl = nullptr;
};
}    // namespace leaf

#endif
