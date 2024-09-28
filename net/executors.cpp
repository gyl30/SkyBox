#include "executors.h"

using leaf::executors;

static void worker(const leaf::executors::executor_ptr &ex)
{
    boost::system::error_code ignore;
    ex->run(ignore);
}

executors::executors(std::size_t executor_size) : executor_size_(executor_size) {}

executors::~executors() { shutdown(); }

void executors::startup()
{
    for (std::size_t i = 0; i < executor_size_; i++)
    {
        auto ex = std::make_shared<leaf::executors::executor>(1);
        workers_.emplace_back(boost::asio::make_work_guard(ex->get_executor()));
        threads_.emplace_back(worker, ex);
        exs_.push_back(ex);
    }
}

void executors::shutdown()
{
    std::lock_guard<std::mutex> const lock(mutex_);
    workers_.clear();
    for (auto &&ex : exs_)
    {
        ex->stop();
    }
    exs_.clear();
    for (auto &&thread : threads_)
    {
        thread.join();
    }
    threads_.clear();
}

boost::asio::io_context &executors::get_executor()
{
    std::lock_guard<std::mutex> const lock(mutex_);
    if (selected_index_ == exs_.size())
    {
        selected_index_ = 0;
    }
    return *exs_[selected_index_++];
}
