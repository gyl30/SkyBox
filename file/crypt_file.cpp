#include <cstddef>
#include "file/crypt_file.h"

namespace leaf
{
void crypt_file::encode(
    leaf::reader* r, leaf::writer* w, encrypt* e, sha256* rs, sha256* ws, boost::system::error_code& ec)
{
    static const auto kBufferSize = 32 * 1024;
    std::vector<uint8_t> plaintext(kBufferSize, '0');
    auto read_size = r->read(plaintext.data(), kBufferSize, ec);
    if (ec)
    {
        return;
    }
    plaintext.resize(read_size);
    rs->update(plaintext.data(), plaintext.size());
    auto ciphertext = e->encode(plaintext, ec);
    if (ec)
    {
        return;
    }
    w->write(ciphertext.data(), ciphertext.size(), ec);
    if (ec)
    {
        return;
    }
    ws->update(ciphertext.data(), ciphertext.size());
}
void crypt_file::decode(
    leaf::reader* r, leaf::writer* w, decrypt* d, sha256* rs, sha256* ws, boost::system::error_code& ec)
{
    static const std::size_t kBufferSize = 32LU * 1024;
    auto read_block_size = kBufferSize + d->padding();
    if (r->size() == 0)
    {
        read_block_size = d->prefix();
    }
    std::vector<uint8_t> ciphertext(read_block_size, '0');
    auto read_size = r->read(ciphertext.data(), read_block_size, ec);
    if (ec)
    {
        return;
    }

    rs->update(ciphertext.data(), read_size);

    ciphertext.resize(read_size);
    auto plaintext = d->decode(ciphertext, ec);
    if (ec)
    {
        return;
    }
    w->write(plaintext.data(), plaintext.size(), ec);
    if (ec)
    {
        return;
    }
    ws->update(plaintext.data(), plaintext.size());
}

}    // namespace leaf
