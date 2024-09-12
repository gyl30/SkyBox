#include <boost/filesystem.hpp>

#include "log.h"
#include "crypt.h"

namespace leaf
{
encrypt_file::encrypt_file(std::string src_file, std::string dst_file, std::vector<uint8_t> key, leaf::executors::executor& ex)
    : reader_(src_file), writer_(dst_file), encrypt_(std::move(key)), src_file_(std::move(src_file)), dst_file_(std::move(dst_file)), ex_(ex) {};

void encrypt_file::startup() { async_start_encrypt(); }
void encrypt_file::shutdown() { async_stop_encrypt(); };
int encrypt_file::process() const { return process_; }

void encrypt_file::async_stop_encrypt()
{
    ex_.post([this]() { stop_encrypt(); });
}
void encrypt_file::stop_encrypt()
{
    boost::system::error_code ec;
    ec = reader_.close();
    if (ec)
    {
        LOG_ERROR("close reader file failed {}", ec.message());
        return;
    }
    ec = writer_.close();
    if (ec)
    {
        LOG_ERROR("close writer file failed {}", ec.message());
        return;
    }
    src_sha_.final();
    dst_sha_.final();
    auto src_hash = src_sha_.hex();
    auto dst_hash = dst_sha_.hex();
    dst_file_size_ = boost::filesystem::file_size(dst_file_, ec);
    if (ec)
    {
        LOG_ERROR("dst file size failed {}", ec.message());
        return;
    }
    LOG_DEBUG("src {} hash {} size {} read size {} dst {} hash {} size {} write size {}", src_file_, src_hash, src_file_size_, read_size_, dst_file_, dst_hash, dst_file_size_, write_size_);
}
void encrypt_file::async_start_encrypt()
{
    boost::system::error_code ec;
    src_file_size_ = boost::filesystem::file_size(src_file_, ec);
    if (ec)
    {
        LOG_ERROR("src file size failed {}", ec.message());
        return;
    }
    ec = reader_.open();
    if (ec)
    {
        LOG_ERROR("open reader file failed {}", ec.message());
        return;
    }
    ec = writer_.open();
    if (ec)
    {
        LOG_ERROR("open writer file failed {}", ec.message());
        return;
    }
    async_encrypt();
}

void encrypt_file::async_encrypt()
{
    ex_.post([this]() { sync_encrypt(); });
}
void encrypt_file::sync_encrypt()
{
    static const auto kBufferSize = 32 * 1024;
    std::vector<uint8_t> plaintext(kBufferSize, '0');
    boost::system::error_code ec;
    auto read_size = reader_.read(plaintext.data(), kBufferSize, ec);
    if (ec)
    {
        LOG_ERROR("read file failed {}", ec.message());
        return;
    }
    read_size_ += read_size;
    plaintext.resize(read_size);
    src_sha_.update(plaintext.data(), plaintext.size());
    auto ciphertext = encrypt_.encode(plaintext);
    if (ciphertext.empty())
    {
        LOG_ERROR("encoder failed {}", read_size);
        return;
    }
    writer_.write(ciphertext.data(), ciphertext.size(), ec);
    if (ec)
    {
        LOG_ERROR("write file failed {}", ec.message());
        return;
    }
    write_size_ += ciphertext.size();
    dst_sha_.update(ciphertext.data(), ciphertext.size());
    double p = static_cast<double>(read_size_) / static_cast<double>(src_file_size_);
    process_ = static_cast<int>(p * 100);
    LOG_DEBUG("{} --> {} process {}", src_file_, dst_file_, process_.load());
    ex_.post([this]() { sync_encrypt(); });
}

decrypt_file::decrypt_file(std::string src_file, std::string dst_file, std::vector<uint8_t> key, leaf::executors::executor& ex)
    : reader_(src_file), writer_(dst_file), decrypt_(std::move(key)), src_file_(std::move(src_file)), dst_file_(std::move(dst_file)), ex_(ex) {};

void decrypt_file::startup() { start_decrypt(); }
void decrypt_file::shutdown() { stop_decrypt(); };
int decrypt_file::process() const { return process_; }

void decrypt_file::start_decrypt() { async_start_decrypt(); }
void decrypt_file::stop_decrypt() { async_stop_decrypt(); }
void decrypt_file::async_stop_decrypt()
{
    ex_.post([this]() { sync_stop_decrypt(); });
}
void decrypt_file::sync_stop_decrypt()
{
    stop_ = true;
    boost::system::error_code ec;
    ec = reader_.close();
    if (ec)
    {
        LOG_ERROR("close reader file failed {}", ec.message());
        return;
    }
    ec = writer_.close();
    if (ec)
    {
        LOG_ERROR("close writer file failed {}", ec.message());
        return;
    }

    src_sha_.final();
    dst_sha_.final();
    auto src_hash = src_sha_.hex();
    auto dst_hash = dst_sha_.hex();
    dst_file_size_ = boost::filesystem::file_size(dst_file_, ec);
    if (ec)
    {
        LOG_ERROR("dst file size failed {}", ec.message());
        return;
    }
    LOG_DEBUG("src {} hash {} size {} read size {} dst {} hash {} size {} write size {}", src_file_, src_hash, src_file_size_, read_size_, dst_file_, dst_hash, dst_file_size_, write_size_);
}

void decrypt_file::async_start_decrypt()
{
    boost::system::error_code ec;
    src_file_size_ = boost::filesystem::file_size(src_file_, ec);
    if (ec)
    {
        LOG_ERROR("src file size failed {}", ec.message());
        return;
    }
    ec = reader_.open();
    if (ec)
    {
        LOG_ERROR("open reader file failed {}", ec.message());
        return;
    }
    ec = writer_.open();
    if (ec)
    {
        LOG_ERROR("open writer file failed {}", ec.message());
        return;
    }
    async_decrypt();
}
void decrypt_file::async_decrypt()
{
    ex_.post([this]() { sync_decrypt(); });
}
void decrypt_file::sync_decrypt()
{
    if (stop_)
    {
        return;
    }

    static const auto kBufferSize = 32 * 1024;
    std::vector<uint8_t> plaintext(kBufferSize, '0');
    boost::system::error_code ec;
    auto read_size = reader_.read(plaintext.data(), kBufferSize, ec);
    if (ec)
    {
        LOG_ERROR("read file failed {}", ec.message());
        return;
    }
    src_sha_.update(plaintext.data(), read_size);
    read_size_ += read_size;

    plaintext.resize(read_size);
    auto ciphertext = decrypt_.decode(plaintext);
    if (ciphertext.empty())
    {
        LOG_ERROR("decoder failed {}", read_size);
        return;
    }

    writer_.write(ciphertext.data(), ciphertext.size(), ec);
    if (ec)
    {
        LOG_ERROR("write file failed {}", ec.message());
        return;
    }
    write_size_ += ciphertext.size();
    dst_sha_.update(ciphertext.data(), ciphertext.size());
    double p = static_cast<double>(read_size_) / static_cast<double>(src_file_size_);
    process_ = static_cast<int>(p * 100);
    LOG_DEBUG("{} --> {} process {}", src_file_, dst_file_, process_.load());
    ex_.post([this]() { sync_decrypt(); });
}

}    // namespace leaf
