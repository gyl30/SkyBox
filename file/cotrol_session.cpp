#include <utility>

#include "file/cotrol_session.h"

namespace leaf
{

cotrol_session::cotrol_session(std::string id, leaf::cotrol_progress_callback cb)
    : id_(std::move(id)), progress_cb_(std::move(cb))
{
}

cotrol_session::~cotrol_session() = default;

void cotrol_session::startup() {}

void cotrol_session::shutdown() {}

void cotrol_session::set_message_cb(std::function<void(std::vector<uint8_t>)> cb) { cb_ = std::move(cb); }

void cotrol_session::on_message(const std::vector<uint8_t> &msg) {}

void cotrol_session::on_message(const leaf::codec_message &msg) {}

}    // namespace leaf
