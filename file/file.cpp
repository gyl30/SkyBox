#include <stack>
#include <climits>
#include <filesystem>
#include <optional>
#include <uv.h>
#include <boost/asio.hpp>
#include <boost/system/error_code.hpp>
#include "file/file.h"
#include "config/config.h"

namespace leaf
{
uint64_t file_id()
{
    static std::atomic<uint64_t> id = 0xff1;
    return ++id;
}

std::string tmp_extension() { return kTmpFilenameSuffix; }

std::string leaf_extension() { return kLeafFilenameSuffix; }

std::string encode_tmp_filename(const std::string& p)
{
    return std::filesystem::path(p).replace_extension(tmp_extension()).string();
}

std::string decode_tmp_filename(const std::string& p) { return std::filesystem::path(p).stem().string(); }

std::string encode_leaf_filename(const std::string& p)
{
    return std::filesystem::path(p).replace_extension(leaf_extension()).string();
}
std::string decode_leaf_filename(const std::string& p) { return std::filesystem::path(p).stem().string(); }

std::string tmp_to_leaf_filename(const std::string& p)
{
    return std::filesystem::path(p).replace_extension(leaf_extension()).string();
}
std::string make_user_path(const std::string& token)
{
    return std::filesystem::path(leaf::kDefaultDir).append(token).string();
}

static std::optional<std::filesystem::path> resolve_abs_path(const std::filesystem::path& root,
                                                             const std::filesystem::path& user_input)
{
    if (user_input.empty())
    {
        return root;
    }
    if (user_input.is_absolute())
    {
        return root;
    }
    std::filesystem::path combined = root / user_input;
    std::filesystem::path resolved = std::filesystem::weakly_canonical(combined);
    std::filesystem::path resolved_root = std::filesystem::weakly_canonical(root);
    if (resolved == resolved_root)
    {
        return resolved;
    }
    std::filesystem::path relative_path = std::filesystem::relative(resolved, resolved_root);
    for (const auto& part : relative_path)
    {
        if (part == "..")
        {
            return {};
        }
    }
    return resolved;
}
std::string make_file_path(const std::string& id, const std::string& filename)
{
    auto dir = std::filesystem::path(leaf::kDefaultDir).append(id);
    boost::system::error_code ec;
    bool exist = std::filesystem::exists(dir, ec);
    if (ec)
    {
        return {};
    }
    if (!exist)
    {
        std::filesystem::create_directories(dir, ec);
        if (ec)
        {
            return {};
        }
    }
    auto file_path = resolve_abs_path(dir, filename);
    if (!file_path.has_value())
    {
        return {};
    }
    auto file_parent_dir = file_path->parent_path();
    exist = std::filesystem::exists(file_parent_dir, ec);
    if (ec)
    {
        return {};
    }
    if (!exist)
    {
        std::filesystem::create_directories(file_parent_dir, ec);
        if (ec)
        {
            return {};
        }
    }
    return file_path->string();
}

std::vector<std::string> dir_files(const std::string& dir)
{
    std::stack<std::string> dirs;
    dirs.push(dir);
    std::vector<std::string> files;
    while (!dirs.empty())
    {
        std::string path = dirs.top();
        dirs.pop();
        for (const auto& entry : std::filesystem::directory_iterator(path))
        {
            if (entry.is_directory())
            {
                dirs.push(entry.path().string());
            }
            if (entry.is_regular_file())
            {
                files.push_back(entry.path().string());
            }
        }
    }
    return files;
}

bool is_dir(const std::string& path) { return std::filesystem::is_directory(path); }

bool is_file(const std::string& file) { return std::filesystem::is_regular_file(file); }

bool rename(const std::string& src, const std::string& dst) { return ::rename(src.c_str(), dst.c_str()) == 0; }

bool remove(const std::string& file) { return ::remove(file.c_str()) == 0; }

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
            flag = UV_FS_O_CREAT | UV_FS_O_RDWR;
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
        return read_at(-1, buffer, size, ec);
    }

    std::size_t read_at(std::int64_t offset, void* buffer, std::size_t size, boost::system::error_code& ec)
    {
        if (size > SSIZE_MAX)
        {
            static constexpr boost::source_location loc = BOOST_CURRENT_LOCATION;
            ec.assign(EINVAL, boost::system::generic_category(), &loc);
            return 0;
        }

        uv_fs_t read_req;
        uv_buf_t buf = uv_buf_init(static_cast<char*>(buffer), size);
        uv_fs_read(nullptr, &read_req, file_, &buf, 1, offset, nullptr);
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
        return write_at(-1, buffer, size, ec);
    }

    std::size_t write_at(std::int64_t offset, void const* buffer, std::size_t size, boost::system::error_code& ec)
    {
        if (size > SSIZE_MAX)
        {
            static constexpr boost::source_location loc = BOOST_CURRENT_LOCATION;
            ec.assign(EINVAL, boost::system::generic_category(), &loc);
            return 0;
        }

        uv_fs_t write_req;
        uv_buf_t buf = uv_buf_init(const_cast<char*>(static_cast<char const*>(buffer)), size);
        uv_fs_write(nullptr, &write_req, file_, &buf, 1, offset, nullptr);
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

std::size_t file_writer::write_at(std::int64_t offset,
                                  void const* buffer,
                                  std::size_t size,
                                  boost::system::error_code& ec)
{
    return impl_->write_at(offset, buffer, size, ec);
}
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

std::size_t file_reader::read_at(std::int64_t offset, void* buffer, std::size_t size, boost::system::error_code& ec)
{
    return impl_->read_at(offset, buffer, size, ec);
}

}    // namespace leaf
