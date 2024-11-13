#include <boost/algorithm/string.hpp>

#include "file_http_handle.h"
#include "upload_file_handle.h"
#include "download_file_handle.h"

namespace leaf
{

leaf::websocket_handle::ptr file_http_handle::websocket_handle(const std::string &id, const std::string &target)
{
    if (boost::ends_with(target, "/upload"))
    {
        return std::make_shared<upload_file_handle>(id);
    }
    if (boost::ends_with(target, "/download"))
    {
        return std::make_shared<download_file_handle>(id);
    }
    return nullptr;
}

void file_http_handle::handle(const leaf::http_session::ptr &session, const leaf::http_session::http_request_ptr &req)
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
