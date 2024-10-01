#ifndef LEAF_WEBSOCKET_HANDLE_H
#define LEAF_WEBSOCKET_HANDLE_H

#include <memory>
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include "websocket_session.h"

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
    virtual void on_text_message(const leaf::websocket_session::ptr&, const std::shared_ptr<std::vector<uint8_t>>&) = 0;
    virtual void on_binary_message(const leaf::websocket_session::ptr&,
                                   const std::shared_ptr<std::vector<uint8_t>>&) = 0;
    virtual void shutdown() = 0;
};

}    // namespace leaf

#endif
