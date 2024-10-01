#ifndef LEAF_WEBSOCKET_SESSION_H
#define LEAF_WEBSOCKET_SESSION_H

#include <memory>
#include <boost/optional.hpp>
#include <boost/asio.hpp>
#include <boost/beast.hpp>

namespace leaf
{

class websocket_session : public std::enable_shared_from_this<websocket_session>
{
   public:
    using ptr = std::shared_ptr<websocket_session>;

   public:
    virtual ~websocket_session() = default;

   public:
    virtual void startup(const boost::beast::http::request<boost::beast::http::string_body>& req) = 0;
    virtual void shutdown() = 0;
    virtual void write(const std::string&) = 0;
};

}    // namespace leaf
#endif
