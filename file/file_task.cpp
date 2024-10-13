#include <random>
#include <boost/filesystem.hpp>

#include "log.h"
#include "chacha20.h"
#include "file_task.h"
#include "crypt_file.h"

static const std::vector<uint8_t> key = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a,
                                         0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15,
                                         0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f};

template <class result_t = std::chrono::milliseconds,
          class clock_t = std::chrono::steady_clock,
          class duration_t = std::chrono::milliseconds>
result_t since(std::chrono::time_point<clock_t, duration_t> const &start)
{
    return std::chrono::duration_cast<result_t>(clock_t::now() - start);
}

std::string random_string(int len)
{
    std::string str("0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz");

    std::random_device rd;

    std::mt19937 generator(rd());

    std::shuffle(str.begin(), str.end(), generator);

    return str.substr(0, len);
}

namespace leaf
{

file_task::file_task(leaf::file_context t) : t_(std::move(t)) {}
file_task::~file_task() = default;

void file_task::startup(boost::system::error_code &ec)
{
    reader_ = new leaf::file_reader(t_.name);
    writer_ = new leaf::file_writer(t_.dst_file);
    reader_sha_ = new leaf::sha256();
    writer_sha_ = new leaf::sha256();
    encrypt_ = new leaf::chacha20_encrypt(key);

    ec = reader_->open();
    if (ec)
    {
        LOG_ERROR("open {} error {}", t_.name, ec.message());
        return;
    }
    ec = writer_->open();
    if (ec)
    {
        LOG_ERROR("open {} error {}", t_.dst_file, ec.message());
        return;
    }
    src_file_size_ = boost::filesystem::file_size(t_.name);
    t_.start_time_ = std::chrono::steady_clock::now();
}
boost::system::error_code file_task::loop()
{
    boost::system::error_code ec;
    leaf::crypt_file::encode(reader_, writer_, encrypt_, reader_sha_, writer_sha_, ec);
    if (ec)
    {
        return ec;
    }
    auto p = static_cast<double>(reader_->size()) / static_cast<double>(src_file_size_);
    t_.progress = static_cast<int>(p * 100);
    return ec;
}
void file_task::shudown(boost::system::error_code &ec)
{
    ec = reader_->close();
    ec = writer_->close();
    reader_sha_->final();
    writer_sha_->final();
    delete reader_;
    delete writer_;
    delete reader_sha_;
    delete writer_sha_;
    delete encrypt_;
}
void file_task::set_error(const boost::system::error_code &ec) { t_.ec = ec; }
boost::system::error_code file_task::error() const { return t_.ec; }
const leaf::file_context &file_task::task_info() const { return t_; }

}    // namespace leaf
