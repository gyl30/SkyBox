#ifndef LEAF_NET_HTTP_SESSION_H
#define LEAF_NET_HTTP_SESSION_H

#include <memory>
#include <boost/optional.hpp>
#include <boost/asio.hpp>
#include <boost/beast.hpp>

namespace leaf
{

class http_session : public std::enable_shared_from_this<http_session>
{
   public:
    using http_response_ptr = std::shared_ptr<boost::beast::http::message_generator>;
    using http_request_ptr = std::shared_ptr<boost::beast::http::request<boost::beast::http::string_body>>;
    using ptr = std::shared_ptr<http_session>;

   public:
    virtual ~http_session() = default;

   public:
    virtual void startup() = 0;
    virtual void shutdown() = 0;
    virtual void write(const http_response_ptr &ptr) = 0;
};

}    // namespace leaf
#endif
