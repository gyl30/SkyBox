#ifndef LEAF_NET_ASYNC_HTTP_H
#define LEAF_NET_ASYNC_HTTP_H

#include <memory>
#include <string>
#include <cstdlib>
#include <functional>
#include <boost/url.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>

#include "log/log.h"
#include "net/executors.h"
#include "net/async_http_client.h"
#include "net/async_https_client.h"

namespace leaf
{

class async_http : public std::enable_shared_from_this<async_http>
{
   public:
    explicit async_http(leaf::executors::executor& ex) : ex_(ex), resolver_(ex) { LOG_DEBUG("async_http constructor"); }
    ~async_http() { LOG_DEBUG("async_http destructor"); }

   public:
    void post(const std::string& url, const std::string& data, const std::function<void(boost::beast::error_code, const std::string&)>& cb)
    {
        cb_ = cb;
        auto parse = boost::urls::parse_uri(url);
        if (!parse)
        {
            cb(make_error_code(boost::system::errc::protocol_error), {});
            return;
        }
        uint16_t port = 0;
        if (parse->has_port())
        {
            port = parse->port_number();
        }
        if (port == 0)
        {
            port = 80;
            if (parse->scheme() == "https")
            {
                port = 443;
            }
        }
        if (parse->scheme() == "https")
        {
            https_ = true;
        }

        std::string target = parse->path();
        std::string host = parse->host();
        req_.version(11);
        req_.method(boost::beast::http::verb::post);
        req_.target(target);
        req_.set(boost::beast::http::field::host, host);
        req_.set(boost::beast::http::field::content_type, "application/json");
        req_.set(boost::beast::http::field::user_agent, "Sailing/X");
        req_.body() = data;
        req_.prepare_payload();
        resolver_.async_resolve(host, std::to_string(port), boost::beast::bind_front_handler(&async_http::on_resolve, shared_from_this()));
    }
    void on_resolve(boost::beast::error_code ec, const boost::asio::ip::tcp::resolver::results_type& results)
    {
        if (ec)
        {
            LOG_ERROR("resolve failed {}", ec.message());
            return;
        }
        if (https_)
        {
            auto client = std::make_shared<leaf::async_https_client>(std::move(req_), ex_);
            client->post(results, cb_);
        }
        else
        {
            auto client = std::make_shared<leaf::async_http_client>(std::move(req_), ex_);
            client->post(results, cb_);
        }
    }

   private:
    bool https_ = false;
    leaf::executors::executor& ex_;
    boost::asio::ip::tcp::resolver resolver_;
    boost::beast::http::request<boost::beast::http::string_body> req_;
    std::function<void(boost::beast::error_code, const std::string&)> cb_;
};
}    // namespace leaf
#endif
