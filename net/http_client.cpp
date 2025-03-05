#include <boost/url.hpp>

#include "log/log.h"
#include "net/socket.h"
#include "crypt/random.h"
#include "net/http_client.h"

namespace leaf
{

http_client::http_client(boost::asio::io_context &io_context)
    : id_(leaf::random_string(8)), stream_(io_context), io_context_(io_context), resolver_(io_context)
{
}

void http_client::post(const std::string &url, const std::string &data, const http_cb &cb)
{
    cb_ = cb;
    auto parse = boost::urls::parse_uri(url);
    if (!parse)
    {
        LOG_ERROR("{} invalid url {}", id_, url);
        call_cb(make_error_code(boost::system::errc::protocol_error), {});
        return;
    }
    std::string host = parse->host();
    uint16_t port = 80;
    if (parse->scheme() == "https")
    {
        port = 443;
    }
    if (parse->has_port())
    {
        port = parse->port_number();
    }
    std::string target = parse->path();
    LOG_INFO("{} url {} host {} port {} target {}", id_, url, host, port, target);
    req_.version(11);
    req_.method(boost::beast::http::verb::post);
    req_.target(target);
    req_.set(boost::beast::http::field::host, host);
    req_.set(boost::beast::http::field::content_type, "application/json");
    req_.set(boost::beast::http::field::user_agent, "leaf/http");
    req_.body() = data;
    req_.prepare_payload();
    boost::system::error_code ec;
    auto addr = boost::asio::ip::make_address(host, ec);
    if (ec)
    {
        LOG_ERROR("{} address failed {}:{}", id_, host, port);
        call_cb(ec, "");
        return;
    }

    auto remote_endpoint = boost::asio::ip::tcp::endpoint(addr, port);
    stream_.expires_after(std::chrono::seconds(30));
    stream_.async_connect(remote_endpoint,
                          boost::beast::bind_front_handler(&http_client::on_connect, shared_from_this()));
}

void http_client::on_connect(boost::beast::error_code ec)
{
    if (ec)
    {
        LOG_ERROR("{} connect failed {}", id_, ec.message());
        call_cb(ec, "");
        shutdown();
        return;
    }

    stream_.expires_after(std::chrono::seconds(30));
    boost::beast::http::async_write(
        stream_, req_, boost::beast::bind_front_handler(&http_client::on_write, shared_from_this()));
}

void http_client::on_write(boost::beast::error_code ec, std::size_t bytes_transferred)
{
    std::string local_addr = get_socket_local_address(stream_.socket());
    std::string remote_addr = get_socket_remote_address(stream_.socket());
    if (ec)
    {
        LOG_ERROR("{} {} write to {} failed {}", id_, local_addr, remote_addr, ec.message());
        call_cb(ec, {});
        shutdown();
        return;
    }
    LOG_DEBUG("{} wirte {} bytes", id_, bytes_transferred);
    stream_.expires_after(std::chrono::seconds(30));
    boost::beast::http::async_read(
        stream_, buffer_, res_, boost::beast::bind_front_handler(&http_client::on_read, shared_from_this()));
}

void http_client::call_cb(boost::beast::error_code ec, const std::string &result)
{
    if (cb_)
    {
        cb_(ec, result);
    }
}
void http_client::on_read(boost::beast::error_code ec, std::size_t bytes_transferred)
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
void http_client::shutdown()
{
    auto self = shared_from_this();
    io_context_.post([this, self]() { safe_shutdown(); });
}

void http_client::safe_shutdown()
{
    if(shutdown_)
    {
        return;
    }
    shutdown_ = true;
    LOG_DEBUG("{} safe shutdown", id_);
    boost::system::error_code ec;
    ec = stream_.socket().close(ec);
}

}    // namespace leaf
