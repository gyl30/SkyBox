#include "tcp_server.h"

namespace leaf
{

tcp_server::tcp_server(handle h, leaf::executors::executor& ex, boost::asio::ip::tcp::endpoint endpoint)
    : handle_(std::move(h)), ex_(ex), endpoint_(std::move(endpoint))
{
}
tcp_server::~tcp_server() = default;

void tcp_server::startup()
{
    auto self = shared_from_this();
    ex_.post([this, self] { safe_startup(); });
}

void tcp_server::safe_startup()
{
    boost::beast::error_code ec;
    ec = acceptor_.open(endpoint_.protocol(), ec);
    if (ec)
    {
        handle_.error(ec);
        return;
    }

    ec = acceptor_.set_option(boost::asio::socket_base::reuse_address(true), ec);
    if (ec)
    {
        handle_.error(ec);
        return;
    }

    ec = acceptor_.bind(endpoint_, ec);
    if (ec)
    {
        handle_.error(ec);
        return;
    }

    ec = acceptor_.listen(boost::asio::socket_base::max_listen_connections, ec);
    if (ec)
    {
        handle_.error(ec);
        return;
    }
    do_accept();
}

void tcp_server::shutdown()
{
    auto self = shared_from_this();
    ex_.post([this, self] { safe_shutdown(); });
}

void tcp_server::safe_shutdown()
{
    boost::system::error_code ec;
    ec = acceptor_.close(ec);
}

void tcp_server::do_accept()
{
    socket_ = handle_.socket();
    acceptor_.async_accept(socket_, boost::beast::bind_front_handler(&tcp_server::on_accept, shared_from_this()));
}

void tcp_server::on_accept(boost::beast::error_code ec)
{
    if (ec)
    {
        handle_.error(ec);
        return;
    }
    handle_.accept(std::move(socket_));
    do_accept();
}

}    // namespace leaf
