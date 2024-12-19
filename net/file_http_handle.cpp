#include <boost/algorithm/string.hpp>

#include "log/log.h"
#include "net/file_http_handle.h"
#include "net/upload_file_handle.h"
#include "net/download_file_handle.h"

namespace leaf
{

leaf::websocket_handle::ptr websocket_handle(const std::string &id, const std::string &target)
{
    LOG_INFO("{} websocket handle target {}", id, target);
    if (boost::ends_with(target, "upload"))
    {
        return std::make_shared<upload_file_handle>(id);
    }
    if (boost::ends_with(target, "download"))
    {
        return std::make_shared<download_file_handle>(id);
    }
    return nullptr;
}

void http_handle(const leaf::http_session::ptr &session, const leaf::http_session::http_request_ptr &req)
{
    boost::beast::http::response<boost::beast::http::string_body> response;
    response.result(boost::beast::http::status::ok);
    response.set(boost::beast::http::field::content_type, "text/plain");
    response.body() = "Hello, world!";
    response.prepare_payload();
    response.keep_alive(req->keep_alive());
    auto msg = std::make_shared<boost::beast::http::message_generator>(std::move(response));
    session->write(msg);
}
}    // namespace leaf
