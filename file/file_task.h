#ifndef LEAF_FILE_TASK_H
#define LEAF_FILE_TASK_H

#include "file.h"
#include "crypt.h"
#include "sha256.h"
#include "file_context.h"

namespace leaf
{

class file_task
{
   public:
    using ptr = std::shared_ptr<file_task>;

   public:
    explicit file_task(leaf::file_context t);
    ~file_task();

   public:
    void startup(boost::system::error_code &ec);
    boost::system::error_code loop();
    void shudown(boost::system::error_code &ec);
    void set_error(const boost::system::error_code &ec);
    [[nodiscard]] boost::system::error_code error() const;
    [[nodiscard]] const leaf::file_context &task_info() const;

   private:
    leaf::reader *reader_ = nullptr;
    leaf::writer *writer_ = nullptr;
    leaf::sha256 *reader_sha_ = nullptr;
    leaf::sha256 *writer_sha_ = nullptr;
    leaf::encrypt *encrypt_ = nullptr;
    uint64_t src_file_size_ = 0;
    leaf::file_context t_;
};

}    // namespace leaf

#endif
