#ifndef LEAF_DETECT_SESSION_H
#define LEAF_DETECT_SESSION_H

#include <memory>
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/asio/ssl.hpp>

#include "http_handle.h"

namespace leaf
{
class detect_session : public std::enable_shared_from_this<detect_session>
{
   public:
    struct handle
    {
        std::function<leaf::http_handle::ptr(void)> http_handle;
    };

   public:
    detect_session(boost::asio::ip::tcp::socket&& socket,
                   boost::asio::ssl::context& ctx,
                   leaf::detect_session::handle h);
    ~detect_session();

    void startup();
    void shutdown();

   private:
    void detect();
    void safe_detect(boost::beast::error_code ec, bool result);
    void safe_shutdown();

   private:
    std::string id_;
    leaf::detect_session::handle h_;
    boost::beast::flat_buffer buffer_;
    boost::beast::tcp_stream stream_;
    boost::asio::ssl::context& ssl_ctx_;
};

}    // namespace leaf

#endif
