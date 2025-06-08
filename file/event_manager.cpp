#include "file/event_manager.h"

namespace leaf
{
void event_manager::startup() { executors_.startup(); }

void event_manager::shutdown() { executors_.shutdown(); }

event_manager& event_manager::instance()
{
    static event_manager instance;
    return instance;
}

void event_manager::subscribe(const std::string& event_name, std::function<void(const std::any&)> cb)
{
    boost::asio::post(executors_.get_executor(), [this, event_name, cb = std::move(cb)]() { cbs_[event_name] = cb; });
}

void event_manager::unsubscribe(const std::string& event_name)
{
    boost::asio::post(executors_.get_executor(), [this, event_name]() { cbs_.erase(event_name); });
}

void event_manager::post(const std::string& event_name, const std::any& data)
{
    boost::asio::post(executors_.get_executor(),
                      [this, event_name, data]()
                      {
                          if (cbs_.contains(event_name))
                          {
                              cbs_[event_name](data);
                          }
                      });
}
}    // namespace leaf
