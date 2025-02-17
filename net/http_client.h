#ifndef LEAF_NET_HTTP_CLIENT_H
#define LEAF_NET_HTTP_CLIENT_H

#include <memory>
#include <boost/optional.hpp>
#include <boost/asio.hpp>
#include <boost/beast.hpp>

namespace leaf
{

class http_client : public std::enable_shared_from_this<http_client>
{
   private:
    using http_cb = std::function<void(boost::beast::error_code, const std::string &)>;

   public:
    explicit http_client(boost::asio::io_context &io_context);
    ~http_client() = default;

   public:
    void post(const std::string &url, const std::string &data, const http_cb &cb);
    void shutdown();

   private:
    void safe_shutdown();
    void on_connect(boost::beast::error_code ec);
    void on_write(boost::beast::error_code ec, std::size_t bytes_transferred);
    void on_read(boost::beast::error_code ec, std::size_t bytes_transferred);
    void call_cb(boost::beast::error_code, const std::string &);

   private:
    http_cb cb_;
    std::string id_;
    boost::beast::tcp_stream stream_;
    boost::beast::flat_buffer buffer_;
    boost::asio::io_context &io_context_;
    boost::asio::ip::tcp::resolver resolver_;
    boost::beast::http::request<boost::beast::http::string_body> req_;
    boost::beast::http::response<boost::beast::http::string_body> res_;
};

}    // namespace leaf

#endif
