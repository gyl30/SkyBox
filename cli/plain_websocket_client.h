#ifndef LEAF_PLAIN_WEBSOCKET_CLIENT_H
#define LEAF_PLAIN_WEBSOCKET_CLIENT_H

#include <queue>
#include <functional>
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include "file_context.h"
#include "file.h"
#include "codec.h"
#include "blake2b.h"

namespace leaf
{
class plain_websocket_client : public std::enable_shared_from_this<plain_websocket_client>
{
   public:
    struct handle
    {
        std::function<void(boost::beast::error_code)> on_connect;
        std::function<void(boost::beast::error_code)> on_close;
    };

   public:
    plain_websocket_client(std::string id, boost::asio::ip::tcp::endpoint ed, boost::asio::io_context& io);
    ~plain_websocket_client();

   public:
    void startup();
    void shutdown();
    void write(const std::vector<uint8_t>& msg);
    void add_file(const leaf::file_context::ptr& file);

   private:
    void on_connect(boost::beast::error_code ec);
    void on_handshake(boost::beast::error_code ec);
    void safe_startup();
    void safe_shutdown();
    void do_read();
    void safe_read();
    void on_read(boost::beast::error_code ec, std::size_t bytes_transferred);

    void safe_add_file(const leaf::file_context::ptr& file);
    void safe_write(const std::vector<uint8_t>& msg);
    void do_write();
    void on_write(boost::beast::error_code ec, std::size_t bytes_transferred);
    void write_message(const codec_message& msg);

   private:
    void create_file_request();
    void create_file_response(const leaf::create_file_response&);
    void delete_file_response(const leaf::delete_file_response&);
    void block_data_request(const leaf::block_data_request&);
    void file_block_request(const leaf::file_block_request&);
    void block_data_finish(const leaf::block_data_finish&);
    void create_file_exist(const leaf::create_file_exist&);
    void error_response(const leaf::error_response&);

   private:
    void open_file();
    void start_timer();
    void timer_callback(const boost::system::error_code& ec);

   private:
    std::string id_;
    boost::beast::flat_buffer buffer_;
    boost::asio::ip::tcp::endpoint ed_;
    leaf::codec_handle codec_;
    leaf::file_context::ptr file_;
    bool writing_ = false;
    std::shared_ptr<leaf::reader> reader_;
    leaf::plain_websocket_client::handle h_;
    std::queue<std::vector<uint8_t>> msg_queue_;
    std::shared_ptr<leaf::blake2b> blake2b_;
    std::shared_ptr<boost::asio::steady_timer> timer_;
    std::queue<leaf::file_context::ptr> padding_files_;
    boost::beast::websocket::stream<boost::beast::tcp_stream> ws_;
};
}    // namespace leaf

#endif
