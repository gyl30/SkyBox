#include "log/log.h"
#include "net/socket.h"
#include "net/tcp_server.h"
#include "net/session_handle.h"
#include "net/detect_session.h"
#include "server/application.h"
#include "file/file_http_handle.h"

namespace leaf
{

application::application(int argc, char *argv[]) : argc_(argc), argv_(argv) {}

application::~application() = default;

void application::startup()
{
    uint16_t listen_port = 8080;
    if (argc_ > 1)
    {
        listen_port = std::atoi(argv_[1]);
    }

    LOG_INFO("listen port {}", listen_port);
    endpoint_ = boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), listen_port);

    leaf::session_handle h2;
    h2.http_handle = leaf::http_handle;
    h2.ws_handle = leaf::websocket_handle;
    leaf::tcp_server::handle h;
    h.accept = [this, h2](boost::asio::ip::tcp::socket socket)
    {
        //
        std::string local_addr = leaf::get_socket_local_address(socket);
        std::string remote_addr = leaf::get_socket_remote_address(socket);
        LOG_INFO("new socket local {} remote {}", local_addr, remote_addr);    // NOLINT
        std::make_shared<leaf::detect_session>(std::move(socket), ssl_ctx_, h2)->startup();
    };

    h.socket = [this]
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
    leaf::set_log_level("trace");
    executors_ = new leaf::executors(4);
    executors_->startup();
    {
        std::atomic<bool> stop{false};
        boost::asio::signal_set sig(executors_->get_executor());
        sig.add(SIGINT);

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

        sig.cancel();
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
