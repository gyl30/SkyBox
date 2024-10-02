#include <boost/asio.hpp>

#include "log.h"
#include "plain_websocket_client.h"

int main(int /*argc*/, char* /*argv*/[])
{
    boost::asio::io_context io(1);
    auto address = boost::asio::ip::address::from_string("127.0.0.1");
    boost::asio::ip::tcp::endpoint ed(address, 8080);
    auto w = std::make_shared<leaf::plain_websocket_client>("ws_cli", ed, io);
    w->startup();
    io.run();
    w->shutdown();
    return 0;
}
