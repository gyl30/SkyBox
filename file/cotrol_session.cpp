#include <utility>
#include "log/log.h"
#include "protocol/codec.h"
#include "protocol/message.h"
#include "file/cotrol_session.h"

namespace leaf
{

cotrol_session::cotrol_session(std::string id,
                               std::string token,
                               leaf::cotrol_handle handler,
                               boost::asio::ip::tcp::endpoint ed,
                               boost::asio::io_context& io)
    : id_(std::move(id)), token_(std::move(token)), io_(io), ed_(std::move(ed)), handler_(std::move(handler))
{
}

cotrol_session::~cotrol_session() = default;

void cotrol_session::startup()
{
    std::string url = "ws://" + ed_.address().to_string() + ":" + std::to_string(ed_.port()) + "/leaf/ws/cotrol";
    ws_client_ = std::make_shared<leaf::plain_websocket_client>(id_, url, ed_, io_);
    ws_client_->set_read_cb([this, self = shared_from_this()](auto ec, const auto& msg) { on_read(ec, msg); });
    ws_client_->set_write_cb([this, self = shared_from_this()](auto ec, std::size_t bytes) { on_write(ec, bytes); });
    ws_client_->set_handshake_cb([this, self = shared_from_this()](auto ec) { on_connect(ec); });
    ws_client_->startup();

    LOG_INFO("{} startup", id_);
}

void cotrol_session::shutdown()
{
    if (ws_client_)
    {
        ws_client_->shutdown();
        ws_client_.reset();
    }
    LOG_INFO("{} shutdown", id_);
}

void cotrol_session::change_current_dir(const std::string& dir) { current_dir_ = dir; }

void cotrol_session::update()
{
    //
    if (ws_client_)
    {
        files_request();
    }
}
void cotrol_session::on_connect(boost::beast::error_code ec)
{
    if (ec)
    {
        shutdown();
        return;
    }
    LOG_INFO("{} connect ws client will login use token {}", id_, token_);
    leaf::login_token lt;
    lt.id = seq_++;
    lt.token = token_;
    ws_client_->write(leaf::serialize_login_token(lt));
}
void cotrol_session::on_read(boost::beast::error_code ec, const std::vector<uint8_t>& bytes)
{
    if (ec)
    {
        shutdown();
        return;
    }
    auto type = get_message_type(bytes);
    if (type == leaf::message_type::files_response)
    {
        on_files_response(leaf::deserialize_files_response(bytes));
    }
}
void cotrol_session::on_write(boost::beast::error_code ec, std::size_t /*transferred*/)
{
    if (ec)
    {
        shutdown();
        return;
    }
}

void cotrol_session::files_request()
{
    leaf::files_request req;
    req.token = token_;
    req.dir = current_dir_;
    ws_client_->write(leaf::serialize_files_request(req));
}

void cotrol_session::on_files_response(const std::optional<leaf::files_response>& files)
{
    if (!files.has_value())
    {
        return;
    }

    LOG_INFO("{} on_files_response {} file size {}", id_, files->token, files->files.size());
    leaf::notify_event e;
    e.method = "files";
    e.data = files->files;
    handler_.notify(e);
}

}    // namespace leaf
