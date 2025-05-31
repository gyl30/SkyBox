#include "net/tcp_server.h"

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
    boost::asio::post(ex_, [this, self] { safe_startup(); });
}

void tcp_server::safe_startup()
{
    acceptor_.open(endpoint_.protocol());
    acceptor_.set_option(boost::asio::socket_base::reuse_address(true));
    acceptor_.bind(endpoint_);
    acceptor_.listen(boost::asio::socket_base::max_listen_connections);
    do_accept();
}

void tcp_server::shutdown()
{
    auto self = shared_from_this();
    boost::asio::post(ex_, [this, self] { safe_shutdown(); });
    while (!shutdown_)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(30));
    }
}

void tcp_server::safe_shutdown()
{
    acceptor_.close();
    shutdown_ = true;
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
