#include <utility>
#include "detect_session.h"
#include "ssl_http_session.h"
#include "plain_http_session.h"

namespace leaf
{

detect_session::detect_session(boost::asio::ip::tcp::socket&& socket, boost::asio::ssl::context& ctx)
    : stream_(std::move(socket)), ssl_ctx_(ctx)
{
}

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
    stream_.expires_after(std::chrono::seconds(30));

    boost::beast::async_detect_ssl(
        stream_, buffer_, boost::beast::bind_front_handler(&detect_session::safe_detect, shared_from_this()));
}

void detect_session::safe_detect(boost::beast::error_code ec, bool result)
{
    if (ec)
    {
        shutdown();
        return;
    }

    if (result)
    {
        std::make_shared<ssl_http_session>(std::move(stream_), ssl_ctx_, std::move(buffer_))->startup();
        return;
    }

    std::make_shared<plain_http_session>(std::move(stream_), std::move(buffer_))->startup();
}
void detect_session::safe_shutdown()
{
    boost::system::error_code ec;
    ec = stream_.socket().close(ec);
}

}    // namespace leaf
