#include "log.h"
#include "socket.h"
#include "tcp_server.h"
#include "application.h"
#include "detect_session.h"

namespace leaf
{

application::application(int argc, char *argv[]) : argc_(argc), argv_(argv) {}

application::~application() = default;

void application::startup()
{
    endpoint_ = boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), 8080);
    leaf::tcp_server::handle h;
    h.accept = [this](boost::asio::ip::tcp::socket socket)
    {
        //
        std::string local_addr = leaf::get_socket_local_address(socket);
        std::string remote_addr = leaf::get_socket_remote_address(socket);
        LOG_INFO("new socket local {} remote {}", local_addr, remote_addr);    // NOLINT
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
        LOG_ERROR("socket error", ec.message());    // NOLINT
        server_->shutdown();
    };

    server_ = std::make_shared<leaf::tcp_server>(h, executors_->get_executor(), endpoint_);
    server_->startup();
}

int application::exec()
{
    leaf::init_log("cmd.log");
    executors_ = new leaf::executors(4);
    executors_->startup();
    {
        std::atomic<bool> stop{false};
        boost::asio::signal_set sig(executors_->get_executor());
        boost::system::error_code ec;
        ec = sig.add(SIGINT, ec);
        if (ec)
        {
            LOG_ERROR("add signal failed", ec.message());
            return -1;
        }

        // 注册退出信号
        sig.async_wait([&stop](boost::system::error_code, int /*s*/) { stop = true; });

        //
        LOG_INFO("start");
        startup();
        //
        while (!stop)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(30));
        }
        //

        ec = sig.cancel(ec);
    }
    LOG_INFO("shutdown");
    shutdown();
    executors_->shutdown();
    delete executors_;
    LOG_INFO("exit");
    leaf::shutdown_log();
    return 0;
}

void application::shutdown()
{
    server_->shutdown();
    server_.reset();
}

}    // namespace leaf
