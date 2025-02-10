#include "file/file_session.h"

namespace leaf
{

bool file_session::is_complete() { return c_ && u_ && d_; }

void file_session::add_cotrol_session(const std::shared_ptr<websocket_session> &c) { c_ = c; }

void file_session::add_upload_session(const std::shared_ptr<websocket_session> &u) { u_ = u; }

void file_session::add_download_session(const std::shared_ptr<websocket_session> &d) { d_ = d; }

}    // namespace leaf
