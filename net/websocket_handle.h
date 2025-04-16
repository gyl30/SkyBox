#ifndef LEAF_NET_WEBSOCKET_HANDLE_H
#define LEAF_NET_WEBSOCKET_HANDLE_H

#include <string>
#include <memory>
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include "net/websocket_session.h"

namespace leaf
{

class websocket_handle : public std::enable_shared_from_this<websocket_handle>
{
   public:
    using ptr = std::shared_ptr<websocket_handle>;

   public:
    virtual ~websocket_handle() = default;

   public:
    virtual void startup() = 0;
    virtual void shutdown() = 0;
    virtual std::string type() const = 0;
};

}    // namespace leaf

#endif
