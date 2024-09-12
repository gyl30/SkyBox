#ifndef LEAF_FILE_H
#define LEAF_FILE_H

#include <string>
#include <boost/system/error_code.hpp>

namespace leaf
{
class file_impl;
class file_writer
{
   public:
    explicit file_writer(std::string filename);
    ~file_writer();

   public:
    boost::system::error_code open();
    boost::system::error_code close();
    std::size_t write(void const* buffer, std::size_t size, boost::system::error_code& ec);

   private:
    file_impl* impl_ = nullptr;
};

class file_reader
{
   public:
    explicit file_reader(std::string filename);
    ~file_reader();

   public:
    boost::system::error_code open();
    boost::system::error_code close();
    std::size_t read(void* buffer, std::size_t size, boost::system::error_code& ec);

   private:
    file_impl* impl_ = nullptr;
};

}    // namespace leaf

#endif
