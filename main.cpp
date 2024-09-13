#include <string>
#include <iostream>
#include <cstdio>
#include <cstdint>
#include <sodium.h>
#include <chrono>
#include <boost/asio.hpp>
#include <boost/filesystem.hpp>
#include "log.h"
#include "file.h"
#include "crypt.h"
#include "chacha20.h"
#include "crypt_file.h"

template <class result_t = std::chrono::milliseconds, class clock_t = std::chrono::steady_clock, class duration_t = std::chrono::milliseconds>
result_t since(std::chrono::time_point<clock_t, duration_t> const& start)
{
    return std::chrono::duration_cast<result_t>(clock_t::now() - start);
}

void encode(const std::string& plain_file, const std::string& encrypt_file, const std::vector<uint8_t>& key, boost::system::error_code& ec)
{
    leaf::reader* r = new leaf::file_reader(plain_file);
    leaf::writer* w = new leaf::file_writer(encrypt_file);
    auto* rs = new leaf::sha256();
    auto* ws = new leaf::sha256();
    leaf::encrypt* e = new leaf::chacha20_encrypt(key);

    ec = r->open();
    if (ec)
    {
        LOG_ERROR("open {} error {}", plain_file, ec.message());
        return;
    }
    ec = w->open();
    if (ec)
    {
        LOG_ERROR("open {} error {}", encrypt_file, ec.message());
        return;
    }
    uint64_t src_file_size = boost::filesystem::file_size(plain_file);
    auto start = std::chrono::steady_clock::now();
    while (!ec)
    {
        leaf::crypt_file::encode(r, w, e, rs, ws, ec);
        if (ec)
        {
            LOG_ERROR("encode error {}", ec.message());
            break;
        }
        // progress
        // auto p = static_cast<double>(r->size()) / static_cast<double>(src_file_size);
        // int progress = static_cast<int>(p * 100);
        // LOG_DEBUG("file szie {} process size {} elapsed {} ms progress {}", src_file_size, r->size(), since(start).count(), progress);
    }
    LOG_DEBUG("encrypt file szie {} process size {} elapsed {} ms", src_file_size, r->size(), since(start).count());
    ec = r->close();
    ec = w->close();
    rs->final();
    ws->final();
    LOG_DEBUG("plain file {} src size {} hash {} encrypt file {} dst size {} hash {}", plain_file, r->size(), rs->hex(), encrypt_file, w->size(), ws->hex());
    delete r;
    delete w;
    delete rs;
    delete ws;
    delete e;
}
void decode(const std::string& encrypt_file, const std::string& plain_file, const std::vector<uint8_t>& key, boost::system::error_code& ec)
{
    leaf::reader* r = new leaf::file_reader(encrypt_file);
    leaf::writer* w = new leaf::file_writer(plain_file);
    auto* rs = new leaf::sha256();
    auto* ws = new leaf::sha256();
    leaf::decrypt* d = new leaf::chacha20_decrypt(key);

    ec = r->open();
    if (ec)
    {
        LOG_ERROR("open {} error {}", encrypt_file, ec.message());
        return;
    }
    ec = w->open();
    if (ec)
    {
        LOG_ERROR("open {} error {}", plain_file, ec.message());
        return;
    }

    uint64_t src_file_size = boost::filesystem::file_size(plain_file);
    auto start = std::chrono::steady_clock::now();

    while (!ec)
    {
        leaf::crypt_file::decode(r, w, d, rs, ws, ec);
    }
    LOG_DEBUG("decrypt file szie {} process size {} elapsed {} ms", src_file_size, r->size(), since(start).count());
    ec = r->close();
    ec = w->close();
    rs->final();
    ws->final();
    LOG_DEBUG("encrypt file {} src size {} hash {} plain file {} dst size {} hash {}", encrypt_file, r->size(), rs->hex(), plain_file, w->size(), ws->hex());
    delete r;
    delete w;
    delete rs;
    delete ws;
    delete d;
}
int main(int argc, char* argv[])
{
    if (argc < 4)
    {
        std::cout << "Usage: " << argv[0] << " plain_file encrypt_file decrypt_file" << std::endl;
        return 0;
    }
    leaf::init_log("log.txt");
    leaf::set_log_level("debug");

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

    boost::system::error_code ec;
    encode(argv[1], argv[2], key, ec);
    if (ec && ec != boost::asio::error::eof)
    {
        LOG_ERROR("{} encode to {} failed", argv[1], argv[2], ec.message());
        return 0;
    }
    decode(argv[2], argv[3], key, ec);
    if (ec)
    {
        LOG_ERROR("{} decode to {} failed", argv[2], argv[3], ec.message());
        return 0;
    }

    leaf::shutdown_log();
    return 0;
}
