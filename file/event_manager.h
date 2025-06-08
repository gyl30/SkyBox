#ifndef LEAF_FILE_EVENT_MANAGER_H
#define LEAF_FILE_EVENT_MANAGER_H

#include <map>
#include <any>
#include <string>
#include <thread>
#include <functional>
#include "net/executors.h"

namespace leaf
{
class event_manager
{
   public:
    event_manager() = default;
    ~event_manager() = default;

   public:
    static event_manager& instance();

   public:
    void startup();
    void shutdown();
    void subscribe(const std::string& event_name, std::function<void(std::any)> cb);
    void unsubscribe(const std::string& event_name);
    void emit(const std::string& event_name, std::any data);

   private:
    std::map<std::string, std::function<void(std::any)>> cbs_;
    std::thread thread_;
    leaf::executors executors_{1};
};
}    // namespace leaf

#endif
