#ifndef LEAF_NET_PLAIN_HTTP_SESSION_H
#define LEAF_NET_PLAIN_HTTP_SESSION_H

#include <boost/optional.hpp>
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include "net/types.h"
#include "net/http_session.h"
#include "net/session_handle.h"

namespace leaf
{

class plain_http_session : public http_session
{
   public:
    plain_http_session(std::string id,
                       tcp_stream_limited&& stream,
                       boost::beast::flat_buffer&& buffer,
                       leaf::session_handle handle);
    ~plain_http_session() override;

   public:
    void startup() override;

    void shutdown() override;

    void write(const http_response_ptr& ptr) override;

   private:
    void safe_write(const http_response_ptr& ptr);
    void on_write(bool keep_alive, boost::beast::error_code ec, std::size_t bytes_transferred);
    void do_read();
    void safe_read();
    void on_read(boost::beast::error_code ec, std::size_t bytes_transferred);
    void safe_shutdown();

   private:
    std::string id_;
    leaf::session_handle handle_;
    boost::beast::flat_buffer buffer_;
    tcp_stream_limited stream_;
    std::shared_ptr<void> self_;
    boost::optional<boost::beast::http::request_parser<boost::beast::http::string_body>> parser_;
};

}    // namespace leaf

#endif
