#ifndef LEAF_NET_PLAIN_WEBSOCKET_CLIENT_H
#define LEAF_NET_PLAIN_WEBSOCKET_CLIENT_H

#include <queue>
#include <atomic>
#include <mutex>
#include <functional>
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/thread/latch.hpp>
#include "net/types.h"
#include "net/websocket_session.h"

namespace leaf
{
class plain_websocket_client : public leaf::websocket_session
{
   public:
    plain_websocket_client(std::string id, std::string host, std::string port, std::string target, boost::asio::io_context& io);
    ~plain_websocket_client();

   public:
    boost::asio::awaitable<void> handshake(boost::beast::error_code&) override;
    boost::asio::awaitable<void> read(boost::beast::error_code&, boost::beast::flat_buffer&) override;
    boost::asio::awaitable<void> write(boost::beast::error_code&, const uint8_t*, std::size_t) override;
    void set_read_limit(std::size_t bytes_per_second) override;
    void set_write_limit(std::size_t bytes_per_second) override;
    void close() override;

   private:
    std::string id_;
    std::string host_;
    std::string port_;
    std::string target_;
    boost::asio::io_context& io_;
    bool connected_ = false;
    std::once_flag shutdown_flag_;
    boost::beast::flat_buffer buffer_;
    std::queue<std::vector<uint8_t>> msg_queue_;
    std::shared_ptr<boost::beast::websocket::stream<tcp_stream_limited>> ws_;
};
}    // namespace leaf

#endif
