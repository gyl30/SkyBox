#ifndef LEAF_PLAIN_HTTP_SESSION_H
#define LEAF_PLAIN_HTTP_SESSION_H

#include <memory>
#include <boost/optional.hpp>
#include <boost/asio.hpp>
#include <boost/beast.hpp>

namespace leaf
{

class plain_http_session : public std::enable_shared_from_this<plain_http_session>
{
   public:
    plain_http_session(std::string id, boost::beast::tcp_stream&& stream, boost::beast::flat_buffer&& buffer);
    ~plain_http_session();

    void startup();
    void shutdown();

   private:
    void do_read();
    void safe_read();
    void on_read(boost::beast::error_code ec, std::size_t bytes_transferred);
    void safe_shutdown();

   private:
    std::string id_;
    boost::beast::flat_buffer buffer_;
    boost::beast::tcp_stream stream_;
    boost::optional<boost::beast::http::request_parser<boost::beast::http::string_body>> parser_;
};

}    // namespace leaf

#endif
