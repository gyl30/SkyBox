#include <utility>
#include "log/log.h"
#include "file/file.h"
#include "file/hash_file.h"

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

void cotrol_session::startup() {}

void cotrol_session::shutdown() {}

void cotrol_session::set_message_cb(std::function<void(std::vector<uint8_t>)> cb) { cb_ = std::move(cb); }

void cotrol_session::on_message(const std::vector<uint8_t>& msg) {}

void cotrol_session::on_message(const leaf::codec_message& msg) {}

void cotrol_session::files_request()
{
    leaf::files_request r;
    r.token = token_.token;
    write_message(r);
}
void cotrol_session::on_files_response(const leaf::files_response& res)
{
    LOG_INFO("{} on_files_response {} file size {}", id_, res.token, res.files.size());
    leaf::notify_event e;
    e.method = "files";
    e.data = res;
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
