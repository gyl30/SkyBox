#include <utility>
#include "log.h"
#include "socket.h"
#include "plain_websocket_client.h"

namespace leaf
{

plain_websocket_client::plain_websocket_client(std::string id,
                                               boost::asio::ip::tcp::endpoint ed,
                                               boost::asio::io_context& io)
    : id_(std::move(id)), ed_(std::move(ed)), ws_(io)
{
    LOG_INFO("create {}", id_);
}

plain_websocket_client::~plain_websocket_client()
{
    //
    LOG_INFO("destroy {}", id_);
}

void plain_websocket_client::startup()
{
    boost::asio::dispatch(ws_.get_executor(),
                          boost::beast::bind_front_handler(&plain_websocket_client::safe_startup, shared_from_this()));
}

void plain_websocket_client::safe_startup()
{
    LOG_INFO("startup {}", id_);
    boost::beast::get_lowest_layer(ws_).expires_after(std::chrono::seconds(3));
    boost::beast::get_lowest_layer(ws_).async_connect(
        ed_, boost::beast::bind_front_handler(&plain_websocket_client::on_connect, shared_from_this()));
}

void plain_websocket_client::on_connect(boost::beast::error_code ec)
{
    if (ec)
    {
        LOG_ERROR("{} connect to {} failed {}", id_, leaf::get_endpoint_address(ed_), ec.message());
        return shutdown();
    }
    boost::beast::get_lowest_layer(ws_).expires_never();

    ws_.set_option(boost::beast::websocket::stream_base::timeout::suggested(boost::beast::role_type::client));
    ws_.set_option(boost::beast::websocket::stream_base::decorator(
        [](boost::beast::websocket::request_type& req) { req.set(boost::beast::http::field::user_agent, "leaf/ws"); }));

    std::string host = leaf::get_endpoint_address(ed_);

    ws_.async_handshake(host,
                        "/leaf/ws/file",
                        boost::beast::bind_front_handler(&plain_websocket_client::on_handshake, shared_from_this()));
}

void plain_websocket_client::on_handshake(boost::beast::error_code ec)
{
    if (ec)
    {
        LOG_ERROR("{} handshake failed {}", id_, ec.message());
        return shutdown();
    }
}

void plain_websocket_client::shutdown()
{
    boost::asio::dispatch(ws_.get_executor(),
                          boost::beast::bind_front_handler(&plain_websocket_client::safe_shutdown, shared_from_this()));
}

void plain_websocket_client::safe_shutdown()
{
    LOG_INFO("shutdown {}", id_);
    boost::beast::error_code ec;
    ec = ws_.next_layer().socket().close(ec);
}

void plain_websocket_client::add_file(const leaf::file_item::ptr& file)
{
    boost::asio::dispatch(
        ws_.get_executor(),
        boost::beast::bind_front_handler(&plain_websocket_client::safe_add_file, shared_from_this(), file));
}

void plain_websocket_client::safe_add_file(const leaf::file_item::ptr& file) { padding_files_.push(file); }

void plain_websocket_client::running_task()
{
    if (running_files_.size() < 3 && !padding_files_.empty())
    {
        running_files_.push_back(padding_files_.front());
        padding_files_.pop();
    }
}
}    // namespace leaf
