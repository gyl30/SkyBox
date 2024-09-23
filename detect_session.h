#ifndef LEAF_DETECT_SESSION_H
#define LEAF_DETECT_SESSION_H

#include <memory>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/beast.hpp>
#include <utility>

namespace leaf
{
class detect_session : public std::enable_shared_from_this<detect_session>
{
   public:
    explicit detect_session(boost::asio::ip::tcp::socket&& socket, boost::asio::ssl::context& ctx, std::string root)
        : root_(std::move(root)), ctx_(ctx), stream_(std::move(socket))
    {
    }

    void run()
    {
        boost::asio::dispatch(stream_.get_executor(),
                              boost::beast::bind_front_handler(&detect_session::on_run, shared_from_this()));
    }

    void on_run()
    {
        stream_.expires_after(std::chrono::seconds(30));

        boost::beast::async_detect_ssl(
            stream_, buffer_, boost::beast::bind_front_handler(&detect_session::on_detect, shared_from_this()));
    }

    void on_detect(boost::beast::error_code ec, bool result)
    {
        if (ec)
        {
        }

        if (result)
        {
            std::make_shared<ssl_http_session>(std::move(stream_), ctx_, std::move(buffer_), root_)->run();
            return;
        }

        std::make_shared<plain_http_session>(std::move(stream_), std::move(buffer_), root_)->run();
    }

   private:
    std::string root_;
    boost::beast::flat_buffer buffer_;
    boost::asio::ssl::context& ctx_;
    boost::beast::tcp_stream stream_;
};

}    // namespace leaf

#endif
