#ifndef LEAF_FILE_FILE_HTTP_HANDLE_H
#define LEAF_FILE_FILE_HTTP_HANDLE_H

#include <string>
#include "net/http_session.h"
#include "net/websocket_handle.h"

namespace leaf
{
void http_handle(const leaf::http_session::ptr &session, const leaf::http_session::http_request_ptr &req);
leaf::websocket_handle::ptr websocket_handle(const std::string &id, const std::string &target);
}    // namespace leaf
#endif
