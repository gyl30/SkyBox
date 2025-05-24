#include <boost/algorithm/string.hpp>

#include "log/log.h"
#include "crypt/passwd.h"
#include "protocol/codec.h"
#include "protocol/message.h"
#include "file/file_session.h"
#include "file/file_http_handle.h"
#include "file/cotrol_file_handle.h"
#include "file/upload_file_handle.h"
#include "file/download_file_handle.h"
#include "file/file_session_manager.h"

namespace leaf
{

leaf::websocket_handle::ptr websocket_handle(const boost::asio::any_io_executor &io,
                                             leaf::websocket_session::ptr &session,
                                             const std::string &id,
                                             const std::string &target)
{
    LOG_INFO("{} websocket handle target {}", id, target);
    if (boost::ends_with(target, "upload"))
    {
        return std::make_shared<upload_file_handle>(io, id, session);
    }
    if (boost::ends_with(target, "download"))
    {
        return std::make_shared<download_file_handle>(io, id, session);
    }
    if (boost::ends_with(target, "cotrol"))
    {
        return std::make_shared<cotrol_file_handle>(io, id, session);
    }
    return nullptr;
}

void http_handle(const leaf::http_session::ptr &session, const leaf::http_session::http_request_ptr &req)
{
    auto target = req->target();
    if (!target.ends_with("login"))
    {
        session->shutdown();
        return;
    }
    std::string data = req->body();
    auto type = leaf::get_message_type(data);
    if (type != leaf::message_type::login)
    {
        session->shutdown();
        return;
    }
    std::vector<uint8_t> data2(data.begin(), data.end());
    auto login = leaf::deserialize_login_request(data2);
    if (!login.has_value())
    {
        session->shutdown();
        return;
    }

    std::string token = leaf::passwd_hash(login->username, login->password);
    leaf::login_token l;
    l.token = token;

    auto s = std::make_shared<leaf::file_session>();
    leaf::fsm::instance().add_session(token, s);

    boost::beast::http::response<boost::beast::http::string_body> response;
    response.result(boost::beast::http::status::ok);
    response.set(boost::beast::http::field::content_type, "text/plain");
    auto token_msg = leaf::serialize_login_token(l);
    response.body().assign(token_msg.begin(), token_msg.end());
    response.prepare_payload();
    response.keep_alive(false);
    auto msg = std::make_shared<boost::beast::http::message_generator>(std::move(response));
    session->write(msg);
}
}    // namespace leaf
