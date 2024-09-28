#ifndef LEAF_PLAIN_HTTP_SESSION_H
#define LEAF_PLAIN_HTTP_SESSION_H

#include <boost/optional.hpp>
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include "http_handle.h"
#include "http_session.h"

namespace leaf
{

class plain_http_session : public http_session
{
   public:
    plain_http_session(std::string id,
                       boost::beast::tcp_stream&& stream,
                       boost::beast::flat_buffer&& buffer,
                       leaf::http_handle::ptr handle);
    ~plain_http_session() override;

    void startup() override;
    void shutdown() override;
    void write(const http_response_ptr& ptr) override;

   private:
    void safe_write(const http_response_ptr& ptr);
    void on_write(boost::beast::error_code ec, std::size_t bytes_transferred);
    void do_read();
    void safe_read();
    void on_read(boost::beast::error_code ec, std::size_t bytes_transferred);
    void safe_shutdown();

   private:
    std::string id_;
    leaf::http_handle::ptr handle_;
    boost::beast::flat_buffer buffer_;
    boost::beast::tcp_stream stream_;
    boost::optional<boost::beast::http::request_parser<boost::beast::http::string_body>> parser_;
};

}    // namespace leaf

#endif
