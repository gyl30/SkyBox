#ifndef LEAF_NET_SSL_WEBSOCKET_SESSION_H
#define LEAF_NET_SSL_WEBSOCKET_SESSION_H

#include <memory>
#include <boost/optional.hpp>
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/ssl/ssl_stream.hpp>
#include "net/types.h"
#include "net/websocket_session.h"

namespace leaf
{
class ssl_websocket_session : public leaf::websocket_session
{
   public:
    explicit ssl_websocket_session(std::string id,
                                   boost::beast::ssl_stream<tcp_stream_limited>&& stream,
                                   boost::beast::http::request<boost::beast::http::string_body> req);
    ~ssl_websocket_session() override;

   public:
    boost::asio::awaitable<void> handshake(boost::beast::error_code& /*unused*/) override;
    boost::asio::awaitable<void> read(boost::beast::error_code& /*unused*/, boost::beast::flat_buffer& /*unused*/) override;
    boost::asio::awaitable<void> write(boost::beast::error_code& /*unused*/, const uint8_t* /*unused*/, std::size_t /*unused*/) override;
    void close() override;

   private:
    std::string id_;
    bool writing_ = false;
    std::shared_ptr<void> self_;
    boost::beast::http::request<boost::beast::http::string_body> req_;
    boost::beast::websocket::stream<boost::beast::ssl_stream<tcp_stream_limited>> ws_;
};

}    // namespace leaf

#endif
