#ifndef LEAF_NET_FILE_HTTP_HANDLE_H
#define LEAF_NET_FILE_HTTP_HANDLE_H

#include <string>
#include "net/http_handle.h"
#include "net/websocket_handle.h"

namespace leaf
{
class file_http_handle : public http_handle
{
   public:
    using ptr = std::shared_ptr<file_http_handle>;

   public:
    file_http_handle() = default;
    ~file_http_handle() override = default;

   public:
    void handle(const leaf::http_session::ptr &session, const leaf::http_session::http_request_ptr &req) override;
    leaf::websocket_handle::ptr websocket_handle(const std::string &id, const std::string &target) override;
};

}    // namespace leaf
#endif