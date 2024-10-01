#include <string>
#include "http_handle.h"
#include "websocket_handle.h"

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
    leaf::websocket_handle::ptr websocket_handle(const std::string & id) override;
};

}    // namespace leaf
