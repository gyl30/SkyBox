#include <utility>
#include "log/log.h"
#include "protocol/codec.h"
#include "protocol/message.h"
#include "file/cotrol_session.h"

namespace leaf
{

cotrol_session::cotrol_session(std::string id,
                               leaf::cotrol_progress_callback cb,
                               leaf::notify_progress_callback notify_cb)
    : id_(std::move(id)), notify_cb_(std::move(notify_cb)), progress_cb_(std::move(cb))
{
}

cotrol_session::~cotrol_session() = default;

void cotrol_session::startup() { LOG_INFO("{} startup", id_); }

void cotrol_session::shutdown() { LOG_INFO("{} shutdown", id_); }

void cotrol_session::set_message_cb(std::function<void(std::vector<uint8_t>)> cb) { cb_ = std::move(cb); }

void cotrol_session::on_message(const std::vector<uint8_t>& bytes)
{
    auto msg = leaf::deserialize_message(bytes.data(), bytes.size());
    if (!msg)
    {
        return;
    }
    leaf::read_buffer r(bytes.data(), bytes.size());
    uint64_t padding = 0;
    r.read_uint64(&padding);
    uint16_t type = 0;
    r.read_uint16(&type);
    if (type == leaf::to_underlying(leaf::message_type::files_response))
    {
        on_files_response(leaf::message::deserialize_files_response(r));
    }
}

void cotrol_session::files_request()
{
    leaf::files_request r;
    r.token = token_.token;
    write_message(r);
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
void cotrol_session::write_message(const codec_message& msg)
{
    if (cb_)
    {
        auto bytes = leaf::serialize_message(msg);
        cb_(std::move(bytes));
    }
}

}    // namespace leaf
