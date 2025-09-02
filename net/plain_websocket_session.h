#ifndef LEAF_NET_PLAIN_WEBSOCKET_SESSION_H
#define LEAF_NET_PLAIN_WEBSOCKET_SESSION_H

#include <queue>
#include <memory>
#include <boost/optional.hpp>
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/beast/websocket.hpp>
#include "net/types.h"
#include "net/websocket_session.h"

namespace leaf
{

class plain_websocket_session : public leaf::websocket_session
{
   public:
    explicit plain_websocket_session(std::string id, tcp_stream_limited&& stream, boost::beast::http::request<boost::beast::http::string_body> req);
    ~plain_websocket_session() override;

   public:
    boost::asio::awaitable<void> handshake(boost::beast::error_code& /*unused*/) override;
    boost::asio::awaitable<void> read(boost::beast::error_code& /*unused*/, boost::beast::flat_buffer& /*unused*/) override;
    boost::asio::awaitable<void> write(boost::beast::error_code& /*unused*/, const uint8_t* /*unused*/, std::size_t /*unused*/) override;
    void set_read_limit(std::size_t bytes_per_second) override;
    void set_write_limit(std::size_t bytes_per_second) override;
    void close() override;

   private:
    std::string id_;
    bool writing_ = false;
    std::shared_ptr<void> self_;
    boost::beast::flat_buffer buffer_;
    std::queue<std::vector<uint8_t>> msg_queue_;
    boost::beast::websocket::stream<tcp_stream_limited> ws_;
    boost::beast::http::request<boost::beast::http::string_body> req_;
};

}    // namespace leaf

#endif
