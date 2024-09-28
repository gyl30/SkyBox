#ifndef LEAF_EXECUTORS_H
#define LEAF_EXECUTORS_H

#include <vector>
#include <memory>
#include <thread>
#include <string>
#include <mutex>
#include <boost/asio.hpp>

namespace leaf
{

class executors
{
   public:
    using executor = boost::asio::io_context;
    using executor_ptr = std::shared_ptr<executor>;

   public:
    explicit executors(std::size_t executor_size);
    ~executors();

   public:
    void startup();
    void shutdown();
    executor &get_executor();

   private:
    using executor_worker = boost::asio::executor_work_guard<executor::executor_type>;
    uint32_t selected_index_ = 0;
    uint32_t executor_size_ = 0;
    std::mutex mutex_;
    std::vector<std::thread> threads_;
    std::vector<executor_ptr> exs_;
    std::vector<executor_worker> workers_;
};
}    // namespace leaf

#endif
