#ifndef LEAF_DETECT_SESSION_H
#define LEAF_DETECT_SESSION_H

#include <memory>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/beast.hpp>

namespace leaf
{
class detect_session : public std::enable_shared_from_this<detect_session>
{
   public:
    explicit detect_session(boost::asio::ip::tcp::socket&& socket, boost::asio::ssl::context& ctx);

    void startup();
    void shutdown();

   private:
    void detect();
    void safe_detect(boost::beast::error_code ec, bool result);
    void safe_shutdown();

   private:
    boost::beast::flat_buffer buffer_;
    boost::beast::tcp_stream stream_;
    boost::asio::ssl::context& ssl_ctx_;
};

}    // namespace leaf

#endif
