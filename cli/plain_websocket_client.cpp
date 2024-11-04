#include <utility>
#include "log.h"
#include "codec.h"
#include "socket.h"
#include "buffer.h"
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
    auto self = shared_from_this();
    uploader_ = std::make_shared<leaf::upload_session>(id_);
    uploader_->set_message_cb(std::bind(&plain_websocket_client::write_message, self, std::placeholders::_1));
    timer_ = std::make_shared<boost::asio::steady_timer>(ws_.get_executor());
    uploader_->startup();
    boost::asio::dispatch(ws_.get_executor(),
                          boost::beast::bind_front_handler(&plain_websocket_client::safe_startup, self));
    start_timer();
}

void plain_websocket_client::safe_startup()
{
    ws_.binary(true);
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
    do_read();
}

void plain_websocket_client::do_read()
{
    boost::asio::dispatch(ws_.get_executor(),
                          boost::beast::bind_front_handler(&plain_websocket_client::safe_read, this));
}
void plain_websocket_client::safe_read()
{
    boost::beast::get_lowest_layer(ws_).expires_after(std::chrono::seconds(30));
    ws_.async_read(buffer_, boost::beast::bind_front_handler(&plain_websocket_client::on_read, this));
}
void plain_websocket_client::on_read(boost::beast::error_code ec, std::size_t bytes_transferred)
{
    if (ec)
    {
        LOG_ERROR("{} read failed {}", id_, ec.message());
        return shutdown();
    }

    LOG_TRACE("{} read message size {}", id_, bytes_transferred);

    auto bytes = leaf::buffers_to_vector(buffer_.data());

    buffer_.consume(buffer_.size());

    auto msg = deserialize_message(bytes->data(), bytes->size());
    if (!msg)
    {
        LOG_ERROR("{} deserialize message error {}", id_, bytes_transferred);
    }
    uploader_->on_message(msg.value());
    do_read();
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
    uploader_->shutdown();
}

void plain_websocket_client::add_file(const leaf::file_context::ptr& file)
{
    boost::asio::dispatch(
        ws_.get_executor(),
        boost::beast::bind_front_handler(&plain_websocket_client::safe_add_file, shared_from_this(), file));
}

void plain_websocket_client::safe_add_file(const leaf::file_context::ptr& file) { uploader_->add_file(file); }

void plain_websocket_client::write(const std::vector<uint8_t>& msg)
{
    boost::asio::dispatch(ws_.get_executor(),
                          boost::beast::bind_front_handler(&plain_websocket_client::safe_write, this, msg));
}

void plain_websocket_client::safe_write(const std::vector<uint8_t>& msg)
{
    assert(!msg.empty());
    msg_queue_.push(msg);
    do_write();
}
void plain_websocket_client::do_write()
{
    if (msg_queue_.empty())
    {
        return;
    }
    if (writing_)
    {
        return;
    }

    writing_ = true;
    auto& msg = msg_queue_.front();

    ws_.async_write(boost::asio::buffer(msg),
                    boost::beast::bind_front_handler(&plain_websocket_client::on_write, this));
}

void plain_websocket_client::on_write(boost::beast::error_code ec, std::size_t bytes_transferred)
{
    if (ec)
    {
        LOG_ERROR("{} write failed {}", id_, ec.message());
        return shutdown();
    }
    LOG_TRACE("{} write success {} bytes", id_, bytes_transferred);
    writing_ = false;
    msg_queue_.pop();
    do_write();
}
void plain_websocket_client::start_timer()
{
    timer_->expires_after(std::chrono::seconds(1));
    timer_->async_wait(std::bind(&plain_websocket_client::timer_callback, this, std::placeholders::_1));
    //
}
void plain_websocket_client::timer_callback(const boost::system::error_code& ec)
{
    if (ec)
    {
        LOG_ERROR("{} timer error {}", id_, ec.message());
        return;
    }
    uploader_->update();
    start_timer();
}

void plain_websocket_client::safe_write_message(const codec_message& msg)
{
    std::vector<uint8_t> bytes;
    if (serialize_message(msg, &bytes) != 0)
    {
        LOG_ERROR("{} serialize message failed", id_);
        return;
    }
    LOG_TRACE("{} send message size {}", id_, bytes.size());
    write(bytes);
}
void plain_websocket_client::write_message(const codec_message& msg)
{
    boost::asio::dispatch(
        ws_.get_executor(),
        boost::beast::bind_front_handler(&plain_websocket_client::safe_write_message, shared_from_this(), msg));
}

}    // namespace leaf
