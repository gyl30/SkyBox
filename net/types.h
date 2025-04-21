#ifndef LEAF_NET_TYPES_H
#define LEAF_NET_TYPES_H

#include <boost/asio.hpp>
#include <boost/beast.hpp>

namespace leaf
{
using tcp_stream_unlimited =
    boost::beast::basic_stream<boost::asio::ip::tcp, boost::asio::any_io_executor, boost::beast::unlimited_rate_policy>;
using tcp_stream_limited =
    boost::beast::basic_stream<boost::asio::ip::tcp, boost::asio::any_io_executor, boost::beast::simple_rate_policy>;

}    // namespace leaf
#endif
