#ifndef LEAF_FILE_FILE_H
#define LEAF_FILE_FILE_H

#include <string>
#include <boost/system/error_code.hpp>

namespace leaf
{
uint64_t file_id();
std::string tmp_extension();
std::string make_tmp_filename(const std::string& p);
std::string make_normal_filename(const std::string& p);
std::string tmp_to_normal_filename(const std::string& p);
std::string make_file_path(const std::string& id, const std::string& filename);
std::string make_file_path(const std::string& id);
std::vector<std::string> dir_files(const std::string& dir);

class file_impl;
class writer
{
   public:
    virtual ~writer() = default;

   public:
    [[nodiscard]] virtual std::string name() const = 0;
    virtual boost::system::error_code open() = 0;
    virtual boost::system::error_code close() = 0;
    virtual std::size_t write(void const* buffer, std::size_t size, boost::system::error_code& ec) = 0;
    virtual std::size_t write_at(std::int64_t offset,
                                 void const* buffer,
                                 std::size_t size,
                                 boost::system::error_code& ec) = 0;
    virtual std::size_t size() = 0;
};
class reader
{
   public:
    virtual ~reader() = default;

   public:
    [[nodiscard]] virtual std::string name() const = 0;
    virtual boost::system::error_code open() = 0;
    virtual boost::system::error_code close() = 0;
    virtual std::size_t read(void* buffer, std::size_t size, boost::system::error_code& ec) = 0;
    virtual std::size_t read_at(std::int64_t offset, void* buffer, std::size_t size, boost::system::error_code& ec) = 0;
    virtual std::size_t size() = 0;
};

class null_writer : public writer
{
   public:
    ~null_writer() override = default;

   public:
    [[nodiscard]] std::string name() const override { return "null_writer"; }
    boost::system::error_code open() override { return {}; }
    boost::system::error_code close() override { return {}; }
    std::size_t write(void const* /*buffer*/, std::size_t size, boost::system::error_code& /*ec*/) override
    {
        size_ += size;
        return size;
    }
    std::size_t write_at(std::int64_t /*offset*/,
                         void const* /*buffer*/,
                         std::size_t size,
                         boost::system::error_code& /*ec*/) override
    {
        size_ += size;
        return size;
    }
    std::size_t size() override { return size_; }

   private:
    std::size_t size_ = 0;
};
class null_reader : public reader
{
   public:
    ~null_reader() override = default;

   public:
    [[nodiscard]] std::string name() const override { return "null_reader"; }
    boost::system::error_code open() override { return {}; }
    boost::system::error_code close() override { return {}; }
    std::size_t read(void* /*buffer*/, std::size_t size, boost::system::error_code& /*ec*/) override
    {
        size_ += size;
        return size;
    }
    std::size_t read_at(std::int64_t /*offset*/,
                        void* /*buffer*/,
                        std::size_t size,
                        boost::system::error_code& /*ec*/) override
    {
        size_ += size;
        return size;
    }
    std::size_t size() override { return size_; }

   private:
    std::size_t size_ = 0;
};

class file_writer : public writer
{
   public:
    explicit file_writer(std::string filename);
    ~file_writer() override;

   public:
    [[nodiscard]] std::string name() const override;
    boost::system::error_code open() override;
    boost::system::error_code close() override;
    std::size_t write(void const* buffer, std::size_t size, boost::system::error_code& ec) override;
    std::size_t write_at(std::int64_t offset,
                         void const* buffer,
                         std::size_t size,
                         boost::system::error_code& ec) override;
    std::size_t size() override;

   private:
    file_impl* impl_ = nullptr;
};

class file_reader : public reader
{
   public:
    explicit file_reader(std::string filename);
    ~file_reader() override;

   public:
    [[nodiscard]] std::string name() const override;
    boost::system::error_code open() override;
    boost::system::error_code close() override;
    std::size_t read(void* buffer, std::size_t size, boost::system::error_code& ec) override;
    std::size_t read_at(std::int64_t offset, void* buffer, std::size_t size, boost::system::error_code& ec) override;
    std::size_t size() override;

   private:
    file_impl* impl_ = nullptr;
};

}    // namespace leaf

#endif
