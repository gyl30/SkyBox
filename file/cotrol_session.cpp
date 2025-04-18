#include <utility>
#include "log/log.h"
#include "protocol/codec.h"
#include "protocol/message.h"
#include "file/cotrol_session.h"

namespace leaf
{

cotrol_session::cotrol_session(std::string id,
                               std::string token,
                               leaf::cotrol_progress_callback cb,
                               leaf::notify_progress_callback notify_cb,
                               boost::asio::ip::tcp::endpoint ed,
                               boost::asio::io_context& io)
    : id_(std::move(id)),
      token_(std::move(token)),
      io_(io),
      ed_(std::move(ed)),
      notify_cb_(std::move(notify_cb)),
      progress_cb_(std::move(cb))
{
}

cotrol_session::~cotrol_session() = default;

void cotrol_session::startup() { LOG_INFO("{} startup", id_); }

void cotrol_session::shutdown() { LOG_INFO("{} shutdown", id_); }

void cotrol_session::set_message_cb(std::function<void(std::vector<uint8_t>)> cb) { cb_ = std::move(cb); }

void cotrol_session::on_message(const std::vector<uint8_t>& bytes)
{
    auto type = get_message_type(bytes);
    if (type == leaf::message_type::files_response)
    {
        on_files_response(leaf::deserialize_files_response(bytes));
    }
}

void cotrol_session::on_files_response(const std::optional<leaf::files_response>& message)
{
    if (!message.has_value())
    {
        return;
    }
    const auto& msg = message.value();

    LOG_INFO("{} on_files_response {} file size {}", id_, msg.token, msg.files.size());
    leaf::notify_event e;
    e.method = "files";
    e.data = msg;
    notify_cb_(e);
}

}    // namespace leaf
