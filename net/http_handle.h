#ifndef LEAF_HTTP_HANDLE_H
#define LEAF_HTTP_HANDLE_H

#include <string>
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include "http_session.h"
#include "websocket_handle.h"

namespace leaf
{

class http_handle
{
   public:
    using ptr = std::shared_ptr<http_handle>;

   public:
    http_handle() = default;
    virtual ~http_handle() = default;

   public:
    virtual void handle(const leaf::http_session::ptr &session, const leaf::http_session::http_request_ptr &req) = 0;
    virtual leaf::websocket_handle::ptr websocket_handle(const std::string &, const std::string &) = 0;
};

}    // namespace leaf

#endif
