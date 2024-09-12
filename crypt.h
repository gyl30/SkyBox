#ifndef LEAF_CRYPT_H
#define LEAF_CRYPT_H

#include <string>
#include "file.h"
#include "sha256.h"
#include "chacha20.h"
#include "executors.h"

namespace leaf
{
class encrypt_file
{
   public:
    encrypt_file(std::string src_file, std::string dst_file, std::vector<uint8_t> key, leaf::executors::executor& ex);
    ~encrypt_file() = default;

   public:
    void startup();
    void shutdown();
    int process() const;

   private:
    void async_stop_encrypt();
    void stop_encrypt();
    void async_start_encrypt();
    void async_encrypt();
    void sync_encrypt();

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
    decrypt_file(std::string src_file, std::string dst_file, std::vector<uint8_t> key, leaf::executors::executor& ex);
    ~decrypt_file() = default;

   public:
    void startup();
    void shutdown();
    int process() const;

   private:
    void start_decrypt();
    void stop_decrypt();
    void async_stop_decrypt();
    void sync_stop_decrypt();

    void async_start_decrypt();
    void async_decrypt();
    void sync_decrypt();

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

}    // namespace leaf

#endif
