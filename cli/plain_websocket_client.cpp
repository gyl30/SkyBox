#include <utility>
#include <filesystem>
#include "log.h"
#include "codec.h"
#include "socket.h"
#include "buffer.h"
#include "message.h"
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
    codec_.create_file_response = std::bind(&plain_websocket_client::create_file_response, self, std::placeholders::_1);
    codec_.delete_file_response = std::bind(&plain_websocket_client::delete_file_response, self, std::placeholders::_1);
    codec_.block_data_request = std::bind(&plain_websocket_client::block_data_request, self, std::placeholders::_1);
    codec_.file_block_request = std::bind(&plain_websocket_client::file_block_request, self, std::placeholders::_1);
    codec_.error_response = std::bind(&plain_websocket_client::error_response, self, std::placeholders::_1);
    boost::asio::dispatch(ws_.get_executor(),
                          boost::beast::bind_front_handler(&plain_websocket_client::safe_startup, self));
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
    do_read();
    process_file();
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

    LOG_DEBUG("{} read message size {}", id_, bytes_transferred);

    auto bytes = leaf::buffers_to_vector(buffer_.data());

    buffer_.consume(buffer_.size());

    deserialize_message(bytes->data(), bytes->size(), &codec_);

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
}

void plain_websocket_client::add_file(const leaf::file_item::ptr& file)
{
    boost::asio::dispatch(
        ws_.get_executor(),
        boost::beast::bind_front_handler(&plain_websocket_client::safe_add_file, shared_from_this(), file));
}

void plain_websocket_client::safe_add_file(const leaf::file_item::ptr& file)
{
    if (file_ != nullptr)
    {
        LOG_INFO("{} change file from {} to {}", id_, file_->name, file->name);
    }
    else
    {
        LOG_INFO("{} add file {}", id_, file->name);
    }

    file_ = file;
    process_file();
}
void plain_websocket_client::process_file()
{
    if (file_ == nullptr)
    {
        return;
    }
    std::error_code ec;
    auto file_size = std::filesystem::file_size(file_->name, ec);
    if (ec)
    {
        LOG_ERROR("{} file size error {} {}", id_, file_->name, ec.message());
        return;
    }
    create_file_request create;
    create.file_size = file_size;
    create.filename = file_->name;
    codec_message msg = create;
    std::vector<uint8_t> bytes;
    if (serialize_message(msg, &bytes) != 0)
    {
        LOG_ERROR("{} serialize create file message error {}", id_, ec.message());
        return;
    }
    bytes.push_back('1');
    LOG_DEBUG("{} send create file {} size {} message size {}", id_, file_->name, file_size, bytes.size());
    this->write(bytes);
}

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
    auto& msg = msg_queue_.front();
    if (msg.back() == '0')
    {
        ws_.text(true);
    }
    else if (msg.back() == '1')
    {
        ws_.binary(true);
    }

    ws_.async_write(boost::asio::buffer(msg.data(), msg.size() - 1),
                    boost::beast::bind_front_handler(&plain_websocket_client::on_write, this));
}

void plain_websocket_client::on_write(boost::beast::error_code ec, std::size_t bytes_transferred)
{
    if (ec)
    {
        LOG_ERROR("{} write failed {}", id_, ec.message());
        return shutdown();
    }
    LOG_DEBUG("{} write success {} bytes", id_, bytes_transferred);
    msg_queue_.pop();
    if (!msg_queue_.empty())
    {
        do_write();
    }
}
void plain_websocket_client::create_file_response(const leaf::create_file_response& msg)
{
    LOG_INFO("{} create file response id {} name {}", id_, msg.file_id, msg.filename);
}
void plain_websocket_client::delete_file_response(const leaf::delete_file_response& msg)
{
    LOG_INFO("{} delete file response name {}", id_, msg.filename);
}
void plain_websocket_client::block_data_request(const leaf::block_data_request& msg)
{
    LOG_INFO("{} block data request id {} block {}", id_, msg.file_id, msg.block_id);
}
void plain_websocket_client::file_block_request(const leaf::file_block_request& msg)
{
    LOG_INFO("{} delete file response file id {}", id_, msg.file_id);
}
void plain_websocket_client::error_response(const leaf::error_response& msg)
{
    LOG_ERROR("{} error {}", id_, msg.error);
}

}    // namespace leaf
