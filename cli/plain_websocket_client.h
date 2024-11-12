#ifndef LEAF_PLAIN_WEBSOCKET_CLIENT_H
#define LEAF_PLAIN_WEBSOCKET_CLIENT_H

#include <queue>
#include <functional>
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include "codec.h"
#include "file_context.h"
#include "upload_session.h"
#include "download_session.h"

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
    void add_upload_file(const leaf::file_context::ptr& file);
    void add_download_file(const leaf::file_context::ptr& file);

   private:
    void on_connect(boost::beast::error_code ec);
    void on_handshake(boost::beast::error_code ec);
    void safe_startup();
    void safe_shutdown();
    void do_read();
    void safe_read();
    void on_read(boost::beast::error_code ec, std::size_t bytes_transferred);

    void safe_add_upload_file(const leaf::file_context::ptr& file);
    void safe_add_download_file(const leaf::file_context::ptr& file);
    void safe_write(const std::vector<uint8_t>& msg);
    void write(const std::vector<uint8_t>& msg);
    void do_write();
    void on_write(boost::beast::error_code ec, std::size_t bytes_transferred);
    void write_message(const codec_message& msg);
    void safe_write_message(const codec_message& msg);

   private:
    void open_file();
    void start_timer();
    void timer_callback(const boost::system::error_code& ec);

   private:
    std::string id_;
    bool writing_ = false;
    boost::beast::flat_buffer buffer_;
    boost::asio::ip::tcp::endpoint ed_;
    std::shared_ptr<upload_session> uploader_;
    std::shared_ptr<download_session> downloader_;
    std::queue<std::vector<uint8_t>> msg_queue_;
    std::shared_ptr<boost::asio::steady_timer> timer_;
    boost::beast::websocket::stream<boost::beast::tcp_stream> ws_;
};
}    // namespace leaf

#endif
