#include <climits>
#include <boost/asio.hpp>
#include <boost/system/error_code.hpp>
#include <filesystem>
#include <uv.h>
#include "file/file.h"

namespace leaf
{
uint64_t file_id()
{
    static std::atomic<uint64_t> id = 0xff1;
    return ++id;
}

std::string make_file_path(const std::string& id) { return std::filesystem::temp_directory_path().append(id).string(); }

std::string make_file_path(const std::string& id, const std::string& filename)
{
    auto dir = std::filesystem::temp_directory_path().append(id);
    boost::system::error_code ec;
    bool exist = std::filesystem::exists(dir, ec);
    if (ec)
    {
        return "";
    }
    if (exist)
    {
        return dir.append(filename).string();
    }
    std::filesystem::create_directories(dir, ec);
    if (ec)
    {
        return "";
    }
    std::string name = dir.append(filename).string();
    return name;
}
class file_impl
{
   public:
    enum class file_operation
    {
        read,
        write

    };

   public:
    explicit file_impl(std::string filename) : filename_(std::move(filename)) {}

    boost::system::error_code open(file_operation op)
    {
        uv_fs_t req;
        int flag = UV_FS_O_RDONLY;
        int mode = 0;
        if (op == file_operation::write)
        {
            flag = UV_FS_O_CREAT | UV_FS_O_RDWR | UV_FS_O_APPEND;
            mode = S_IREAD | S_IWRITE;
        }
        uv_fs_open(nullptr, &req, filename_.c_str(), flag, mode, nullptr);
        boost::system::error_code ec;
        if (req.result < 0)
        {
            static constexpr boost::source_location loc = BOOST_CURRENT_LOCATION;
            int err = uv_translate_sys_error(static_cast<int>(req.result));
            ec.assign(err, boost::system::system_category(), &loc);
            uv_fs_req_cleanup(&req);
            return ec;
        }
        uv_fs_req_cleanup(&req);
        file_ = static_cast<int>(req.result);
        return ec;
    }

    boost::system::error_code close()
    {
        uv_fs_t close_req;
        uv_fs_close(nullptr, &close_req, file_, nullptr);
        boost::system::error_code ec;
        if (close_req.result < 0)
        {
            static constexpr boost::source_location loc = BOOST_CURRENT_LOCATION;
            int err = uv_translate_sys_error(static_cast<int>(close_req.result));
            ec.assign(err, boost::system::generic_category(), &loc);
            uv_fs_req_cleanup(&close_req);
            return ec;
        }
        uv_fs_req_cleanup(&close_req);
        file_ = -1;
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

        uv_fs_t read_req;
        uv_buf_t buf = uv_buf_init(static_cast<char*>(buffer), size);
        uv_fs_read(nullptr, &read_req, file_, &buf, 1, -1, nullptr);
        if (read_req.result < 0)
        {
            static constexpr boost::source_location loc = BOOST_CURRENT_LOCATION;
            int err = uv_translate_sys_error(static_cast<int>(read_req.result));
            ec.assign(err, boost::system::generic_category(), &loc);
            uv_fs_req_cleanup(&read_req);
            return 0;
        }
        if (read_req.result == 0)
        {
            uv_fs_req_cleanup(&read_req);
            ec = boost::asio::error::eof;
            return 0;
        }
        auto read_size = static_cast<std::size_t>(read_req.result);
        read_size_ += read_req.result;
        uv_fs_req_cleanup(&read_req);
        ec = {};
        return read_size;
    }

    std::size_t write(void const* buffer, std::size_t size, boost::system::error_code& ec)
    {
        if (size > SSIZE_MAX)
        {
            static constexpr boost::source_location loc = BOOST_CURRENT_LOCATION;
            ec.assign(EINVAL, boost::system::generic_category(), &loc);
            return 0;
        }

        uv_fs_t write_req;
        uv_buf_t buf = uv_buf_init(const_cast<char*>(static_cast<char const*>(buffer)), size);
        uv_fs_write(nullptr, &write_req, file_, &buf, 1, -1, nullptr);
        if (write_req.result < 0)
        {
            static constexpr boost::source_location loc = BOOST_CURRENT_LOCATION;
            int err = uv_translate_sys_error(static_cast<int>(write_req.result));
            ec.assign(err, boost::system::system_category(), &loc);
            uv_fs_req_cleanup(&write_req);
            return 0;
        }
        auto write_size = static_cast<std::size_t>(write_req.result);
        write_size_ += write_req.result;
        uv_fs_req_cleanup(&write_req);
        return write_size;
    }
    std::size_t read_size() const { return read_size_; }
    std::size_t write_size() const { return write_size_; }
    std::string name() const { return filename_; }

   private:
    uv_file file_ = -1;
    std::size_t read_size_ = 0;
    std::size_t write_size_ = 0;
    std::string filename_;
};
//
file_writer::file_writer(std::string filename) : impl_(new file_impl(std::move(filename))) {}
file_writer::~file_writer() { delete impl_; }
boost::system::error_code file_writer::open() { return impl_->open(file_impl::file_operation::write); }
boost::system::error_code file_writer::close() { return impl_->close(); }
std::size_t file_writer::write(void const* buffer, std::size_t size, boost::system::error_code& ec)
{
    return impl_->write(buffer, size, ec);
}
std::size_t file_writer::size() { return impl_->write_size(); };
std::string file_writer::name() const { return impl_->name(); };
//
file_reader::file_reader(std::string filename) : impl_(new file_impl(std::move(filename))) {}
file_reader::~file_reader() { delete impl_; }
boost::system::error_code file_reader::open() { return impl_->open(file_impl::file_operation::read); }
boost::system::error_code file_reader::close() { return impl_->close(); }
std::size_t file_reader::read(void* buffer, std::size_t size, boost::system::error_code& ec)
{
    return impl_->read(buffer, size, ec);
}
std::size_t file_reader::size() { return impl_->read_size(); };
std::string file_reader::name() const { return impl_->name(); };

}    // namespace leaf
