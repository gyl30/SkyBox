#include "log.h"
#include "tcp_server.h"
#include "application.h"
#include "detect_session.h"

namespace leaf
{

application::application() = default;

application::~application() = default;

void application::startup()
{
    executors_ = new leaf::executors(4);
    endpoint_ = boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), 8080);
    leaf::tcp_server::handle h;
    h.accept = [this](boost::asio::ip::tcp::socket socket)
    {
        //
        std::make_shared<leaf::detect_session>(std::move(socket), ssl_ctx_)->startup();
    };

    h.socket = [this]()
    {
        //
        return boost::asio::ip::tcp::socket(executors_->get_executor());
    };
    h.error = [this](boost::beast::error_code ec)
    {
        //
        server_->shutdown();
    };

    server_ = new leaf::tcp_server(h, executors_->get_executor(), endpoint_);
}

void application::exec() {}

void application::shutdown()
{
    executors_->shutdown();
    delete server_;
    delete executors_;
}

}    // namespace leaf
