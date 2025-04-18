#ifndef LEAF_NET_WEBSOCKET_SESSION_H
#define LEAF_NET_WEBSOCKET_SESSION_H

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
    using read_cb = std::function<void(boost::beast::error_code, const std::vector<uint8_t>&)>;
    using write_cb = std::function<void(boost::beast::error_code, std::size_t)>;
    using handshake_cb = std::function<void(boost::beast::error_code)>;
    virtual void startup() = 0;
    virtual void shutdown() = 0;
    virtual void write(const std::vector<uint8_t>& msg) = 0;
    virtual void set_read_cb(read_cb) = 0;
    virtual void set_write_cb(write_cb) = 0;
    virtual void set_handshake_cb(handshake_cb) = 0;
};

}    // namespace leaf
#endif
