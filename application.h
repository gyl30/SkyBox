#ifndef LEAF_APPLICATION_H
#define LEAF_APPLICATION_H

#include "tcp_server.h"

namespace leaf
{
class application
{
   public:
    application(int argc, char** argv);
    ~application();

   public:
    void exec();

   private:
    int argc_;
    char** argv_;
    leaf::tcp_server* server_;
};
}    // namespace leaf

#endif
