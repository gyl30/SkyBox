#include <boost/asio/use_awaitable.hpp>
#include <boost/asio/redirect_error.hpp>

#include "log/log.h"
#include "net/plain_websocket_client.h"

namespace leaf
{

plain_websocket_client::plain_websocket_client(std::string id, std::string host, std::string port, std::string target, boost::asio::io_context& io)
    : id_(std::move(id)), host_(std::move(host)), port_(std::move(port)), target_(std::move(target)), io_(io)
{
    LOG_INFO("create {}", id_);
}

plain_websocket_client::~plain_websocket_client()
{
    //
    LOG_INFO("destroy {}", id_);
}
boost::asio::awaitable<void> plain_websocket_client::handshake(boost::beast::error_code& ec)
{
    auto resolver = boost::asio::use_awaitable_t<boost::asio::any_io_executor>::as_default_on(
        boost::asio::ip::tcp::resolver(co_await boost::asio::this_coro::executor));

    auto const results = co_await resolver.async_resolve(host_, port_);

    ws_ = std::make_shared<boost::beast::websocket::stream<tcp_stream_limited>>(
        boost::asio::use_awaitable_t<boost::asio::any_io_executor>::as_default_on(
            boost::beast::websocket::stream<tcp_stream_limited>(co_await boost::asio::this_coro::executor)));

    auto ep = co_await boost::beast::get_lowest_layer(*ws_).async_connect(results, boost::asio::redirect_error(boost::asio::use_awaitable, ec));
    if (ec)
    {
        co_return;
    }

    std::string host = host_ + ':' + std::to_string(ep.port());

    boost::beast::get_lowest_layer(*ws_).expires_never();

    ws_->set_option(boost::beast::websocket::stream_base::timeout::suggested(boost::beast::role_type::client));
    ws_->set_option(boost::beast::websocket::stream_base::decorator([](auto& req) { req.set(boost::beast::http::field::user_agent, "leaf/ws"); }));

    co_await ws_->async_handshake(host, target_, boost::asio::redirect_error(boost::asio::use_awaitable, ec));
}

boost::asio::awaitable<void> plain_websocket_client::read(boost::beast::error_code& ec, boost::beast::flat_buffer& buffer)
{
    co_await ws_->async_read(buffer, boost::asio::redirect_error(boost::asio::use_awaitable, ec));
}

boost::asio::awaitable<void> plain_websocket_client::write(boost::beast::error_code& ec, const uint8_t* data, std::size_t data_size)
{
    co_await ws_->async_write(boost::asio::buffer(data, data_size), boost::asio::redirect_error(boost::asio::use_awaitable, ec));
}

void plain_websocket_client::close()
{
    if (ws_ != nullptr)
    {
        ws_->next_layer().close();
        ws_.reset();
    }
}
}    // namespace leaf
