#ifndef LEAF_SERVER_APPLICATION_H
#define LEAF_SERVER_APPLICATION_H

#include "net/tcp_server.h"

namespace leaf
{
class application
{
   public:
    application(int argc, char* argv[]);
    ~application();

   public:
    int exec();

   private:
    void startup();
    void shutdown();

   private:
    int argc_ = 0;
    char** argv_ = nullptr;
    leaf::executors* executors_ = nullptr;
    boost::asio::ip::tcp::endpoint endpoint_;
    std::shared_ptr<leaf::tcp_server> server_;
    boost::asio::ssl::context ssl_ctx_{boost::asio::ssl::context::tls_server};
};
}    // namespace leaf

#endif
