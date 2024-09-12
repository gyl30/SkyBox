#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <climits>
#include <boost/asio.hpp>    // eof

#include "file.h"

namespace leaf
{
class file_impl
{
   public:
    explicit file_impl(std::string filename) : filename_(std::move(filename)) {}

    boost::system::error_code open(int flag, int mode)
    {
        boost::system::error_code ec;
        int fd = ::open(filename_.c_str(), flag, mode);
        if (fd < 0)
        {
            static constexpr boost::source_location loc = BOOST_CURRENT_LOCATION;
            ec.assign(errno, boost::system::system_category(), &loc);
        }
        else
        {
            fd_ = fd;
        }
        return ec;
    }

    boost::system::error_code close()
    {
        boost::system::error_code ec;
        if (fd_ < 0)
        {
            static constexpr boost::source_location loc = BOOST_CURRENT_LOCATION;
            ec.assign(EINVAL, boost::system::generic_category(), &loc);
        }
        else
        {
            ::close(fd_);
            fd_ = -1;
        }
        return ec;
    }

    std::size_t read(void* buffer, std::size_t size, boost::system::error_code& ec)
    {
        if (size > SSIZE_MAX)
        {
            static constexpr boost::source_location loc = BOOST_CURRENT_LOCATION;
            ec.assign(EINVAL, boost::system::generic_category(), &loc);
            return 0;
        }

        ssize_t r = ::read(fd_, buffer, size);

        if (r < 0)
        {
            static constexpr boost::source_location loc = BOOST_CURRENT_LOCATION;
            ec.assign(errno, boost::system::system_category(), &loc);
            return 0;
        }
        if (r == 0)
        {
            ec = boost::asio::error::eof;
            return 0;
        }

        read_size_ += r;
        ec = {};
        return r;
    }

    std::size_t write(void const* buffer, std::size_t size, boost::system::error_code& ec)
    {
        if (size > SSIZE_MAX)
        {
            static constexpr boost::source_location loc = BOOST_CURRENT_LOCATION;
            ec.assign(EINVAL, boost::system::generic_category(), &loc);
            return 0;
        }

        ssize_t r = ::write(fd_, buffer, size);

        if (r < 0)
        {
            static constexpr boost::source_location loc = BOOST_CURRENT_LOCATION;
            ec.assign(errno, boost::system::system_category(), &loc);
            return 0;
        }

        ec = {};
        if (r < static_cast<ssize_t>(size))
        {
            static constexpr boost::source_location loc = BOOST_CURRENT_LOCATION;
            ec.assign(ENOSPC, boost::system::system_category(), &loc);
        }
        write_size_ += r;
        return r;
    }
    std::size_t read_size() const { return read_size_; }
    std::size_t write_size() const { return write_size_; }

   private:
    int fd_ = -1;
    std::size_t read_size_ = 0;
    std::size_t write_size_ = 0;
    std::string filename_;
};
//
file_writer::file_writer(std::string filename) : impl_(new file_impl(std::move(filename))) {}
file_writer::~file_writer() { delete impl_; }
boost::system::error_code file_writer::open() { return impl_->open(O_WRONLY | O_CREAT | O_TRUNC, 0644); }
boost::system::error_code file_writer::close() { return impl_->close(); }
std::size_t file_writer::write(void const* buffer, std::size_t size, boost::system::error_code& ec) { return impl_->write(buffer, size, ec); }
std::size_t file_writer::size() { return impl_->write_size(); };
//
file_reader::file_reader(std::string filename) : impl_(new file_impl(std::move(filename))) {}
file_reader::~file_reader() { delete impl_; }
boost::system::error_code file_reader::open() { return impl_->open(O_RDONLY, 0); }
boost::system::error_code file_reader::close() { return impl_->close(); }
std::size_t file_reader::read(void* buffer, std::size_t size, boost::system::error_code& ec) { return impl_->read(buffer, size, ec); }
std::size_t file_reader::size() { return impl_->read_size(); };

}    // namespace leaf
