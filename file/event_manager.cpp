#include "file/event_manager.h"

namespace leaf
{
void event_manager::startup() { executors_.startup(); }

void event_manager::shutdown() { executors_.shutdown(); }

void event_manager::subscribe(const std::string& event_name, std::function<void(std::any)> cb)
{
    boost::asio::post(executors_.get_executor(), [this, event_name, cb = std::move(cb)]() { cbs_[event_name] = cb; });
}

void event_manager::unsubscribe(const std::string& event_name)
{
    boost::asio::post(executors_.get_executor(), [this, event_name]() { cbs_.erase(event_name); });
}

void event_manager::emit(const std::string& event_name, std::any data)
{
    boost::asio::post(executors_.get_executor(),
                      [this, event_name, data = std::move(data)]()
                      {
                          if (cbs_.contains(event_name))
                          {
                              cbs_[event_name](data);
                          }
                      });
}
}    // namespace leaf
