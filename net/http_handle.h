#ifndef LEAF_HTTP_HANDLE_H
#define LEAF_HTTP_HANDLE_H

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include "http_session.h"

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
};

}    // namespace leaf

#endif
