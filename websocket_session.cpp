#include "websocket_session.h"

namespace leaf
{

websocket_session::websocket_session(leaf::websocket_session::handle h, boost::asio::ip::tcp::socket socket)
    : handle_(std::move(h)), ws_(std::move(socket))
{
}

websocket_session::~websocket_session() = default;

void websocket_session::startup()
{
    //
    boost::asio::dispatch(ws_.get_executor(),
                          boost::beast::bind_front_handler(&websocket_session::safe_startup, shared_from_this()));
}

void websocket_session::shutdown()
{
    boost::asio::dispatch(ws_.get_executor(),
                          boost::beast::bind_front_handler(&websocket_session::safe_shutdown, shared_from_this()));
}

void websocket_session::safe_startup()
{
    ws_.set_option(boost::beast::websocket::stream_base::timeout::suggested(boost::beast::role_type::server));

    ws_.set_option(boost::beast::websocket::stream_base::decorator(
        [](boost::beast::websocket::response_type& res) { res.set(boost::beast::http::field::server, "leaf/ws"); }));

    do_accept();
}

void websocket_session::safe_shutdown() {}

void websocket_session::write(const std::shared_ptr<std::string>& msg)
{
    boost::asio::dispatch(ws_.get_executor(),
                          boost::beast::bind_front_handler(&websocket_session::safe_write, shared_from_this(), msg));
}

void websocket_session::safe_write(const std::shared_ptr<std::string>& msg)
{
    queue_.push(msg);
    do_write();
}

void websocket_session::do_accept()
{
    ws_.async_accept(boost::beast::bind_front_handler(&websocket_session::on_accept, shared_from_this()));
}

void websocket_session::on_accept(boost::beast::error_code ec)
{
    if (ec)
    {
        handle_.handshake_error(ec);
        return;
    }
    do_read();
}

void websocket_session::do_read()
{
    ws_.async_read(buffer_, boost::beast::bind_front_handler(&websocket_session::on_read, shared_from_this()));
}

void websocket_session::on_read(boost::beast::error_code ec, std::size_t transferred)
{
    boost::ignore_unused(transferred);
    if (ec)
    {
        handle_.read_error(ec);
        return;
    }
    const auto buffer = boost::beast::buffers_to_string(buffer_.data());
    if (ws_.got_binary())
    {
        handle_.binary(buffer);
    }
    else
    {
        handle_.text(buffer);
    }
    do_read();
}

void websocket_session::do_write()
{
    ws_.async_write(boost::asio::buffer(*queue_.front()),
                    boost::beast::bind_front_handler(&websocket_session::on_write, shared_from_this()));
}

void websocket_session::on_write(boost::beast::error_code ec, std::size_t transferred)
{
    boost::ignore_unused(transferred);
    if (ec)
    {
        handle_.write_error(ec);
        return;
    }
    queue_.pop();
    if (!queue_.empty())
    {
        do_write();
    }
}

}    // namespace leaf
