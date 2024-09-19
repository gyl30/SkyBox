#ifndef LEAF_FILE_TASK_H
#define LEAF_FILE_TASK_H

#include "log.h"
#include "file.h"
#include "crypt.h"
#include "chacha20.h"
#include "task_item.h"
#include "crypt_file.h"

namespace leaf
{

class file_task
{
   public:
    using ptr = std::shared_ptr<file_task>;

   public:
    explicit file_task(leaf::task_item t);
    ~file_task();

   public:
    void startup(boost::system::error_code &ec);
    boost::system::error_code loop();
    void shudown(boost::system::error_code &ec);
    void set_error(const boost::system::error_code &ec);
    boost::system::error_code error() const;
    const leaf::task_item &task_info() const;

   private:
    leaf::reader *reader_ = nullptr;
    leaf::writer *writer_ = nullptr;
    leaf::sha256 *reader_sha_ = nullptr;
    leaf::sha256 *writer_sha_ = nullptr;
    leaf::encrypt *encrypt_ = nullptr;
    uint64_t src_file_size_ = 0;
    leaf::task_item t_;
};

}    // namespace leaf

#endif
