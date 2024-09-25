#ifndef LEAF_APPLICATION_H
#define LEAF_APPLICATION_H

#include "tcp_server.h"

namespace leaf
{
class application
{
   public:
    application();
    ~application();

   public:
    void startup();
    void exec();
    void shutdown();

   private:
    leaf::tcp_server* server_ = nullptr;
    leaf::executors* executors_ = nullptr;
    boost::asio::ip::tcp::endpoint endpoint_;
    boost::asio::ssl::context ssl_ctx_{boost::asio::ssl::context::tls_server};
};
}    // namespace leaf

#endif
