#include <boost/system.hpp>

#include "net/socket.h"

namespace leaf
{
template <typename Ed>
std::string get_endpoint_address_(const Ed& ed)
{
    const std::string ip = ed.address().to_string();
    const uint16_t port = ed.port();
    return ip + ":" + std::to_string(port);
}

template <typename Socket>
static std::string get_socket_local_ip_(Socket& socket)
{
    boost::system::error_code ec;
    auto ed = socket.local_endpoint(ec);
    if (ec)
    {
        return "";
    }
    std::string ip = ed.address().to_string();
    return ip;
}
template <typename Socket>
static uint16_t get_socket_local_port_(Socket& socket)
{
    boost::system::error_code ec;
    auto ed = socket.local_endpoint(ec);
    if (ec)
    {
        return 0;
    }
    return ed.port();
}
//
template <typename Socket>
static std::string get_socket_remote_ip_(Socket& socket)
{
    boost::system::error_code ec;
    auto ed = socket.remote_endpoint(ec);
    if (ec)
    {
        return "";
    }
    std::string ip = ed.address().to_string();
    return ip;
}
template <typename Socket>
static uint16_t get_socket_remote_port_(Socket& socket)
{
    boost::system::error_code ec;
    auto ed = socket.remote_endpoint(ec);
    if (ec)
    {
        return 0;
    }
    return ed.port();
}
std::string get_socket_local_ip(boost::asio::ip::udp::socket& socket) { return get_socket_local_ip_(socket); }

uint16_t get_socket_local_port(boost::asio::ip::udp::socket& socket) { return get_socket_local_port_(socket); }

std::string get_socket_remote_ip(boost::asio::ip::udp::socket& socket) { return get_socket_remote_ip_(socket); }
uint16_t get_socket_remote_port(boost::asio::ip::udp::socket& socket) { return get_socket_remote_port_(socket); }
std::string get_socket_local_ip(boost::asio::ip::tcp::socket& socket) { return get_socket_local_ip_(socket); }
uint16_t get_socket_local_port(boost::asio::ip::tcp::socket& socket) { return get_socket_local_port_(socket); }
std::string get_socket_remote_ip(boost::asio::ip::tcp::socket& socket) { return get_socket_remote_ip_(socket); }
uint16_t get_socket_remote_port(boost::asio::ip::tcp::socket& socket) { return get_socket_remote_port_(socket); }
std::string get_endpoint_address(const boost::asio::ip::tcp::endpoint& ed) { return get_endpoint_address_(ed); }
std::string get_endpoint_address(const boost::asio::ip::udp::endpoint& ed) { return get_endpoint_address_(ed); }

std::string get_socket_remote_address(boost::asio::ip::tcp::socket& socket)
{
    boost::system::error_code ec;
    auto ed = socket.remote_endpoint(ec);
    if (ec)
    {
        return "";
    }
    return get_endpoint_address(ed);
}
std::string get_socket_local_address(boost::asio::ip::tcp::socket& socket)
{
    boost::system::error_code ec;
    auto ed = socket.local_endpoint(ec);
    if (ec)
    {
        return "";
    }
    return get_endpoint_address(ed);
}
std::string get_socket_local_address(boost::asio::ip::udp::socket& socket)
{
    boost::system::error_code ec;
    auto ed = socket.local_endpoint(ec);
    if (ec)
    {
        return "";
    }
    return get_endpoint_address(ed);
}
std::string get_socket_remote_address(boost::asio::ip::udp::socket& socket)
{
    boost::system::error_code ec;
    auto ed = socket.remote_endpoint(ec);
    if (ec)
    {
        return "";
    }
    return get_endpoint_address(ed);
}

boost::asio::ip::tcp::socket change_socket_io_context(boost::asio::ip::tcp::socket sock, boost::asio::io_context& io)
{
    std::string local = get_socket_local_address(sock);
    std::string remote = get_socket_remote_address(sock);
    boost::system::error_code ec;
    boost::asio::ip::tcp::socket tmp(io);
    auto fd = sock.release(ec);
    if (ec)
    {
        return tmp;
    }
    tmp.assign(boost::asio::ip::tcp::v4(), fd);
    return tmp;
}
}    // namespace leaf
