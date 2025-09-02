#ifndef LEAF_NET_WEBSOCKET_SESSION_H
#define LEAF_NET_WEBSOCKET_SESSION_H

#include <memory>
#include <boost/optional.hpp>
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/asio/use_awaitable.hpp>

namespace leaf
{

class websocket_session : public std::enable_shared_from_this<websocket_session>
{
   public:
    using ptr = std::shared_ptr<websocket_session>;

   public:
    virtual ~websocket_session() = default;

   public:
    virtual void close() = 0;
    virtual boost::asio::awaitable<void> handshake(boost::beast::error_code&) = 0;
    virtual boost::asio::awaitable<void> read(boost::beast::error_code&, boost::beast::flat_buffer&) = 0;
    virtual boost::asio::awaitable<void> write(boost::beast::error_code&, const uint8_t*, std::size_t) = 0;
    virtual void set_read_limit(std::size_t bytes_per_second) = 0;
    virtual void set_write_limit(std::size_t bytes_per_second) = 0;
};

}    // namespace leaf
#endif
