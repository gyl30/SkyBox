#include <utility>
#include <boost/asio/buffer.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/asio/redirect_error.hpp>
#include <boost/beast/core/buffers_range.hpp>
#include "log/log.h"
#include "net/buffer.h"
#include "net/plain_websocket_session.h"

namespace leaf
{

plain_websocket_session::plain_websocket_session(std::string id,
                                                 tcp_stream_limited&& stream,
                                                 boost::beast::http::request<boost::beast::http::string_body> req)
    : id_(std::move(id)), ws_(std::move(stream)), req_(std::move(req))
{
    LOG_INFO("create {}", id_);
}

plain_websocket_session::~plain_websocket_session()
{
    //
    LOG_INFO("destroy {}", id_);
}
boost::asio::awaitable<void> plain_websocket_session::handshake(boost::beast::error_code& ec)
{
    ws_.set_option(boost::beast::websocket::stream_base::timeout::suggested(boost::beast::role_type::server));

    ws_.set_option(boost::beast::websocket::stream_base::decorator([](boost::beast::websocket::response_type& res)
                                                                   { res.set(boost::beast::http::field::server, "leaf/ws"); }));

    co_return co_await ws_.async_accept(req_, boost::asio::redirect_error(boost::asio::use_awaitable, ec));
}

boost::asio::awaitable<void> plain_websocket_session::read(boost::beast::error_code& ec, boost::beast::flat_buffer& buffer)
{
    //

    co_await ws_.async_read(buffer, boost::asio::redirect_error(boost::asio::use_awaitable, ec));
}

boost::asio::awaitable<void> plain_websocket_session::write(boost::beast::error_code& ec, const uint8_t* data, std::size_t data_len)
{
    //
    co_await ws_.async_write(boost::asio::buffer(data, data_len), boost::asio::redirect_error(boost::asio::use_awaitable, ec));
}
void plain_websocket_session::close()
{
    if (ws_.is_open())
    {
        boost::beast::get_lowest_layer(ws_).close();
    }
}

}    // namespace leaf
