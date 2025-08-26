#include <boost/url.hpp>

#include "log/log.h"
#include "net/socket.h"
#include "crypt/random.h"
#include "net/async_http_client.h"

namespace leaf
{

async_http_client::async_http_client(boost::beast::http::request<boost::beast::http::string_body> req, leaf::executors::executor& ex)
    : id_(random_string(16)), ex_(ex), stream_(ex), resolver_(ex), req_(std::move(req))
{
    LOG_DEBUG("{} create", id_);
}

async_http_client::~async_http_client() { LOG_DEBUG("{} destroy", id_); }

void async_http_client::post(const boost::asio::ip::tcp::resolver::results_type& results, const http_cb& cb)
{
    cb_ = cb;
    stream_.expires_after(std::chrono::seconds(30));
    stream_.async_connect(results, boost::beast::bind_front_handler(&async_http_client::on_connect, shared_from_this()));
}

void async_http_client::on_connect(const boost::beast::error_code& ec, const boost::asio::ip::tcp::resolver::results_type::endpoint_type&)
{
    if (ec)
    {
        LOG_ERROR("{} connect failed {}", id_, ec.message());
        call_cb(ec, "");
        shutdown();
        return;
    }

    stream_.expires_after(std::chrono::seconds(30));
    boost::beast::http::async_write(stream_, req_, boost::beast::bind_front_handler(&async_http_client::on_write, shared_from_this()));
}

void async_http_client::on_write(const boost::beast::error_code& ec, std::size_t bytes_transferred)
{
    std::string local_addr = get_socket_local_address(stream_.socket());
    std::string remote_addr = get_socket_remote_address(stream_.socket());
    if (ec)
    {
        LOG_ERROR("{} {} write to {} failed {}", id_, local_addr, remote_addr, ec.message());

        call_cb(ec, "");
        shutdown();
        return;
    }
    LOG_DEBUG("{} wirte {} bytes", id_, bytes_transferred);
    stream_.expires_after(std::chrono::seconds(30));
    boost::beast::http::async_read(stream_, buffer_, res_, boost::beast::bind_front_handler(&async_http_client::on_read, shared_from_this()));
}

void async_http_client::call_cb(const boost::beast::error_code& ec, const std::string& result)
{
    if (cb_)
    {
        cb_(ec, result);
    }
}
void async_http_client::on_read(const boost::beast::error_code& ec, std::size_t bytes_transferred)
{
    boost::ignore_unused(bytes_transferred);

    std::string local_addr = get_socket_local_address(stream_.socket());
    std::string remote_addr = get_socket_remote_address(stream_.socket());
    if (ec)
    {
        LOG_ERROR("{} {} read from {} failed {}", id_, local_addr, remote_addr, ec.message());
    }
    call_cb(ec, res_.body());
    shutdown();
}
void async_http_client::shutdown()
{
    auto self = shared_from_this();
    ex_.post([this, self]() { safe_shutdown(); });
}

void async_http_client::safe_shutdown()
{
    cb_ = nullptr;
    LOG_DEBUG("{} safe shutdown", id_);
    boost::system::error_code ec;
    ec = stream_.socket().close(ec);
}
}    // namespace leaf
