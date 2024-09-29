#include "log.h"
#include "plain_http_session.h"
#include "plain_websocket_session.h"

namespace leaf
{

plain_http_session::plain_http_session(std::string id,
                                       boost::beast::tcp_stream&& stream,
                                       boost::beast::flat_buffer&& buffer,
                                       leaf::http_handle::ptr handle)
    : id_(std::move(id)), handle_(std::move(handle)), buffer_(std::move(buffer)), stream_(std::move(stream))
{
    LOG_INFO("create {}", id_);
}
plain_http_session::~plain_http_session() { LOG_INFO("destroy {}", id_); }

void plain_http_session::startup()
{
    LOG_INFO("startup {}", id_);
    self_ = shared_from_this();
    do_read();
}

void plain_http_session::shutdown()
{
    boost::asio::dispatch(stream_.get_executor(),
                          boost::beast::bind_front_handler(&plain_http_session::safe_shutdown, this));
}

void plain_http_session::do_read()
{
    boost::asio::dispatch(stream_.get_executor(),
                          boost::beast::bind_front_handler(&plain_http_session::safe_read, this));
}

void plain_http_session::safe_read()
{
    parser_.emplace();

    parser_->body_limit(10000);

    boost::beast::get_lowest_layer(stream_).expires_after(std::chrono::seconds(30));

    boost::beast::http::async_read(
        stream_, buffer_, *parser_, boost::beast::bind_front_handler(&plain_http_session::on_read, this));
}

void plain_http_session::on_read(boost::beast::error_code ec, std::size_t bytes_transferred)
{
    boost::ignore_unused(bytes_transferred);

    if (ec)
    {
        return shutdown();
    }

    if (boost::beast::websocket::is_upgrade(parser_->get()))
    {
        boost::beast::get_lowest_layer(stream_).expires_never();
        return std::make_shared<leaf::plain_websocket_session>(std::move(stream_))->startup(parser_->release());
    }
    auto req_ptr = std::make_shared<boost::beast::http::request<boost::beast::http::string_body>>(parser_->release());
    handle_->handle(shared_from_this(), req_ptr);
}

void plain_http_session::write(const http_response_ptr& ptr)
{
    boost::asio::dispatch(stream_.get_executor(),
                          boost::beast::bind_front_handler(&plain_http_session::safe_write, this, ptr));
}

void plain_http_session::safe_write(const http_response_ptr& ptr)
{
    bool keep_alive = ptr->keep_alive();
    boost::beast::http::message_generator msg(std::move(*ptr));
    boost::beast::async_write(stream_,
                              std::move(msg),    //
                              boost::beast::bind_front_handler(&plain_http_session::on_write, this, keep_alive));
}

void plain_http_session::on_write(bool keep_alive, boost::beast::error_code ec, std::size_t bytes_transferred)
{
    if (keep_alive)
    {
        do_read();
    }
    else
    {
        shutdown();
    }
}

void plain_http_session::safe_shutdown()
{
    LOG_INFO("shutdown {}", id_);
    boost::beast::error_code ec;
    ec = stream_.socket().close(ec);

    std::shared_ptr<void> self = self_;
    self_.reset();
    boost::asio::dispatch(stream_.get_executor(), [self]() mutable { self.reset(); });
}

}    // namespace leaf
