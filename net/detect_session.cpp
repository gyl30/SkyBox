#include <utility>
#include "log/log.h"
#include "net/socket.h"
#include "config/config.h"
#include "net/detect_session.h"
#include "net/ssl_http_session.h"
#include "net/plain_http_session.h"

namespace leaf
{

detect_session::detect_session(boost::asio::ip::tcp::socket&& socket,
                               boost::asio::ssl::context& ctx,
                               leaf::session_handle h)
    : h_(std::move(h)), stream_(std::move(socket)), ssl_ctx_(ctx)
{
    id_ = leaf::get_socket_remote_address(stream_.socket());
    LOG_INFO("create {}", id_);
}

detect_session::~detect_session() { LOG_INFO("destroy {}", id_); }

void detect_session::startup()
{
    boost::asio::dispatch(stream_.get_executor(),
                          boost::beast::bind_front_handler(&detect_session::detect, shared_from_this()));
}
void detect_session::shutdown()
{
    auto self = shared_from_this();
    boost::asio::dispatch(stream_.get_executor(),
                          boost::beast::bind_front_handler(&detect_session::safe_shutdown, self));
}

void detect_session::detect()
{
    LOG_INFO("detect {}", id_);
    stream_.rate_policy().read_limit(kReadWsLimited);
    stream_.rate_policy().write_limit(kWriteWsLimited);
    stream_.expires_after(std::chrono::seconds(30));

    boost::beast::async_detect_ssl(
        stream_, buffer_, boost::beast::bind_front_handler(&detect_session::safe_detect, shared_from_this()));
}

void detect_session::safe_detect(boost::beast::error_code ec, bool result)
{
    if (ec)
    {
        LOG_ERROR("detect {} failed {}", id_, ec.message());
        shutdown();
        return;
    }

    if (result)
    {
        LOG_INFO("detect ssl success {}", id_);
        std::make_shared<ssl_http_session>(id_, std::move(stream_), ssl_ctx_, std::move(buffer_), h_)->startup();
        return;
    }

    LOG_INFO("detect plain success {}", id_);
    std::make_shared<plain_http_session>(id_, std::move(stream_), std::move(buffer_), h_)->startup();
}

void detect_session::safe_shutdown()
{
    LOG_INFO("shutdown {}", id_);
    boost::system::error_code ec;
    ec = stream_.socket().close(ec);
}

}    // namespace leaf
