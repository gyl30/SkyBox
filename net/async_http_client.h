#ifndef LEAF_NET_ASYNC_HTTP_CLIENT_H
#define LEAF_NET_ASYNC_HTTP_CLIENT_H

#include <memory>
#include <string>
#include <cstdlib>
#include <functional>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>

#include "net/executors.h"

namespace leaf
{

class async_http_client : public std::enable_shared_from_this<async_http_client>
{
   private:
    using http_cb = std::function<void(boost::beast::error_code, const std::string&)>;

   public:
    async_http_client(boost::beast::http::request<boost::beast::http::string_body> req, leaf::executors::executor& ex);
    ~async_http_client();

   public:
    void post(const boost::asio::ip::tcp::resolver::results_type& results, const http_cb& cb);
    void shutdown();

   private:
    void safe_shutdown();
    void on_resolve(const boost::beast::error_code& ec, const boost::asio::ip::tcp::resolver::results_type& results);
    void on_connect(const boost::beast::error_code& ec, const boost::asio::ip::tcp::resolver::results_type::endpoint_type&);
    void on_write(const boost::beast::error_code& ec, std::size_t bytes_transferred);
    void on_read(const boost::beast::error_code& ec, std::size_t bytes_transferred);
    void call_cb(const boost::beast::error_code& ec, const std::string&);

   private:
    std::string id_;
    http_cb cb_;
    leaf::executors::executor& ex_;
    boost::beast::tcp_stream stream_;
    boost::beast::flat_buffer buffer_;
    boost::asio::ip::tcp::resolver resolver_;
    boost::beast::http::request<boost::beast::http::string_body> req_;
    boost::beast::http::response<boost::beast::http::string_body> res_;
};
}    // namespace leaf

#endif
