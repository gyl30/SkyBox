#include <utility>
#include <filesystem>
#include "log.h"
#include "codec.h"
#include "socket.h"
#include "buffer.h"
#include "message.h"
#include "blake2b.h"
#include "hash_file.h"
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
    codec_.block_data_finish = std::bind(&plain_websocket_client::block_data_finish, self, std::placeholders::_1);
    boost::asio::dispatch(ws_.get_executor(),
                          boost::beast::bind_front_handler(&plain_websocket_client::safe_startup, self));
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

    LOG_DEBUG("{} read message size {}", id_, bytes_transferred);

    auto bytes = leaf::buffers_to_vector(buffer_.data());

    buffer_.consume(buffer_.size());

    if (deserialize_message(bytes->data(), bytes->size(), &codec_) != 0)
    {
        LOG_ERROR("{} deserialize message error {}", id_, bytes_transferred);
    }

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

void plain_websocket_client::add_file(const leaf::file_context::ptr& file)
{
    boost::asio::dispatch(
        ws_.get_executor(),
        boost::beast::bind_front_handler(&plain_websocket_client::safe_add_file, shared_from_this(), file));
}

void plain_websocket_client::safe_add_file(const leaf::file_context::ptr& file)
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
    create_file_request();
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
    LOG_DEBUG("{} write success {} bytes", id_, bytes_transferred);
    writing_ = false;
    msg_queue_.pop();
    do_write();
}
void plain_websocket_client::create_file_response(const leaf::create_file_response& msg)
{
    file_->id = msg.file_id;
    LOG_INFO("{} create_file_response id {} name {}", id_, msg.file_id, msg.filename);
}
void plain_websocket_client::delete_file_response(const leaf::delete_file_response& msg)
{
    LOG_INFO("{} delete_file_response name {}", id_, msg.filename);
}

void plain_websocket_client::create_file_request()
{
    if (file_ == nullptr)
    {
        return;
    }
    boost::system::error_code hash_ec;
    std::string h = leaf::hash_file(file_->name, hash_ec);
    if (hash_ec)
    {
        LOG_ERROR("{} create_file_request file {} hash error {}", id_, file_->name, hash_ec.message());
        return;
    }
    std::error_code size_ec;
    auto file_size = std::filesystem::file_size(file_->name, size_ec);
    if (size_ec)
    {
        LOG_ERROR("{} create_file_request file {} size error {}", id_, file_->name, size_ec.message());
        return;
    }
    open_file();
    if (reader_ == nullptr)
    {
        return;
    }
    LOG_DEBUG("{} create_file_request {} size {} hash {}", id_, file_->name, file_size, h);
    leaf::create_file_request create;
    create.file_size = file_size;
    create.hash = h;
    create.filename = std::filesystem::path(file_->name).filename().string();
    file_->file_size = file_size;
    write_message(create);
}
void plain_websocket_client::open_file()
{
    assert(file_ && !reader_ && !blake2b_);

    reader_ = std::make_shared<leaf::file_reader>(file_->name);
    auto ec = reader_->open();
    if (ec)
    {
        LOG_ERROR("{} file open error {}", id_, ec.message());
        return;
    }
    blake2b_ = std::make_shared<leaf::blake2b>();
}

void plain_websocket_client::close_file()
{
    assert(file_ && reader_ && blake2b_);
    blake2b_->final();
    LOG_INFO("{} close file {} hex {}", id_, file_->name, blake2b_->hex());
    file_ = nullptr;
    blake2b_.reset();
    auto r = reader_;
    reader_ = nullptr;
    auto ec = r->close();
    if (ec)
    {
        LOG_ERROR("{} close file error {}", id_, ec.message());
        return;
    }
}
void plain_websocket_client::block_data_request(const leaf::block_data_request& msg)
{
    assert(file_ && reader_ && blake2b_);
    LOG_INFO("{} block_data_request id {} block {}", id_, msg.file_id, msg.block_id);
    if (msg.block_id < file_->send_block_count)
    {
        LOG_ERROR("{} block_data_request block {} less than send block {}", id_, msg.block_id, file_->send_block_count);
        return;
    }

    boost::system::error_code ec;
    std::vector<uint8_t> buffer(file_->block_size, '0');
    auto read_size = reader_->read(buffer.data(), buffer.size(), ec);
    if (ec)
    {
        LOG_ERROR("{} block_data_request {} error {}", id_, msg.block_id, ec.message());
        return;
    }
    blake2b_->update(buffer.data(), read_size);
    buffer.resize(read_size);
    file_->send_block_count = msg.block_id;
    LOG_DEBUG("{} block_data_request {} size {}", id_, msg.block_id, read_size);
    leaf::block_data_response response;
    response.file_id = msg.file_id;
    response.block_id = msg.block_id;
    response.data = std::move(buffer);
    write_message(response);
}
void plain_websocket_client::file_block_request(const leaf::file_block_request& msg)
{
    if (msg.file_id != file_->id)
    {
        LOG_ERROR("{} file id not match {} {}", id_, msg.file_id, file_->id);
        return;
    }
    constexpr auto kBlockSize = 128 * 1024;
    uint64_t block_count = file_->file_size / kBlockSize;
    if (file_->file_size % kBlockSize != 0)
    {
        block_count++;
    }
    leaf::file_block_response response;
    response.block_count = block_count;
    response.block_size = kBlockSize;
    response.file_id = file_->id;
    file_->block_size = kBlockSize;
    LOG_INFO("{} file_block_request id {} count {} size {}", id_, file_->id, block_count, kBlockSize);
    write_message(response);
}

void plain_websocket_client::block_data_finish(const leaf::block_data_finish& msg)
{
    LOG_INFO("{} block_data_finish id {} file {} hash {}", id_, msg.file_id, msg.filename, msg.hash);
    close_file();
}

void plain_websocket_client::write_message(const codec_message& msg)
{
    std::vector<uint8_t> bytes;
    if (serialize_message(msg, &bytes) != 0)
    {
        LOG_ERROR("{} serialize message failed", id_);
        return;
    }
    LOG_DEBUG("{} send message size {}", id_, bytes.size());
    write(bytes);
}

void plain_websocket_client::error_response(const leaf::error_response& msg)
{
    LOG_ERROR("{} error {}", id_, msg.error);
}

}    // namespace leaf
