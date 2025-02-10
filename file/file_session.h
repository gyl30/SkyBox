#ifndef LEAF_FILE_FILE_SESSION
#define LEAF_FILE_FILE_SESSION

#include "net/websocket_session.h"

namespace leaf
{

class file_session
{
   public:
    file_session() = default;
    ~file_session() = default;

   public:
    bool is_complete();
    void add_upload_session(const std::shared_ptr<websocket_session> &u);
    void add_download_session(const std::shared_ptr<websocket_session> &d);
    void add_cotrol_session(const std::shared_ptr<websocket_session> &c);

   private:
    std::shared_ptr<websocket_session> c_;
    std::shared_ptr<websocket_session> u_;
    std::shared_ptr<websocket_session> d_;
};

}    // namespace leaf

#endif
