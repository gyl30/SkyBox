#include <string>
#include <utility>
#include <atomic>
#include <chrono>
#include <iostream>
#include <cstdio>
#include <cstdint>
#include <sodium.h>
#include <boost/filesystem.hpp>
#include "log.h"
#include "file.h"
#include "sha256.h"
#include "chacha20.h"
#include "executors.h"

class encrypt_file
{
   public:
    encrypt_file(std::string src_file, std::string dst_file, std::vector<uint8_t> key, leaf::executors::executor& ex)
        : reader_(src_file), writer_(dst_file), encrypt_(std::move(key)), src_file_(std::move(src_file)), dst_file_(std::move(dst_file)), ex_(ex) {};
    ~encrypt_file() = default;

   public:
    void startup() { async_start_encrypt(); }
    void shutdown() { async_stop_encrypt(); };
    int process() const { return process_; }

   private:
    void async_stop_encrypt()
    {
        ex_.post([this]() { stop_encrypt(); });
    }
    void stop_encrypt()
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
        LOG_DEBUG("src {} hash {} size {} read size {} dst {} hash {} size {} write size {}", src_file_, src_hash, src_file_size_, read_size_, dst_file_, src_hash, dst_file_size_, write_size_);
    }
    void async_start_encrypt()
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

    void async_encrypt()
    {
        ex_.post([this]() { sync_encrypt(); });
    }
    void sync_encrypt()
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

   private:
    leaf::file_reader reader_;
    leaf::file_writer writer_;
    leaf::chacha20_encrypt encrypt_;
    std::string src_file_;
    std::string dst_file_;
    leaf::sha256 src_sha_;
    leaf::sha256 dst_sha_;
    uint64_t src_file_size_ = 0;
    uint64_t dst_file_size_ = 0;
    uint64_t read_size_ = 0;
    uint64_t write_size_ = 0;
    std::atomic<int> process_{0};
    leaf::executors::executor& ex_;
};
class decrypt_file
{
   public:
    decrypt_file(std::string src_file, std::string dst_file, std::vector<uint8_t> key, leaf::executors::executor& ex)
        : reader_(src_file), writer_(dst_file), decrypt_(std::move(key)), src_file_(std::move(src_file)), dst_file_(std::move(dst_file)), ex_(ex) {};
    ~decrypt_file() = default;

   public:
    void startup() { start_decrypt(); }
    void shutdown() { stop_decrypt(); };
    int process() const { return process_; }

   private:
    void start_decrypt() { async_start_decrypt(); }
    void stop_decrypt() { async_stop_decrypt(); }
    void async_stop_decrypt()
    {
        ex_.post([this]() { sync_stop_decrypt(); });
    }
    void sync_stop_decrypt()
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
        LOG_DEBUG("src {} hash {} size {} read size {} dst {} hash {} size {} write size {}", src_file_, src_hash, src_file_size_, read_size_, dst_file_, src_hash, dst_file_size_, write_size_);
    }

    void async_start_decrypt()
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
    void async_decrypt()
    {
        ex_.post([this]() { sync_decrypt(); });
    }
    void sync_decrypt()
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

   private:
    bool stop_ = false;
    leaf::file_reader reader_;
    leaf::file_writer writer_;
    leaf::chacha20_decrypt decrypt_;
    std::string src_file_;
    std::string dst_file_;
    leaf::sha256 src_sha_;
    leaf::sha256 dst_sha_;
    uint64_t read_size_ = 0;
    uint64_t src_file_size_ = 0;
    uint64_t write_size_ = 0;
    uint64_t dst_file_size_ = 0;
    std::atomic<int> process_{0};
    leaf::executors::executor& ex_;
};

int main(int argc, char* argv[])
{
    if (argc < 4)
    {
        std::cout << "Usage: " << argv[0] << " plain_file encrypt_file decrypt_file" << std::endl;
        return 0;
    }
    leaf::init_log("log.txt");
    leaf::set_log_level("debug");

    leaf::executors executors(4);
    executors.startup();

    const static std::vector<uint8_t> key = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
                                             0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f};
    if (boost::filesystem::exists(argv[2]))
    {
        boost::filesystem::remove(argv[2]);
    }
    if (boost::filesystem::exists(argv[3]))
    {
        boost::filesystem::remove(argv[3]);
    }

    encrypt_file e(argv[1], argv[2], key, executors.get_executor());
    e.startup();
    std::this_thread::sleep_for(std::chrono::seconds(3));
    e.shutdown();
    decrypt_file d(argv[2], argv[3], key, executors.get_executor());
    d.startup();
    std::this_thread::sleep_for(std::chrono::seconds(3));
    d.shutdown();
    //
    std::this_thread::sleep_for(std::chrono::milliseconds(30));

    executors.shutdown();
    leaf::shutdown_log();
    return 0;
}
