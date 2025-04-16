#ifndef LEAF_FILE_COTROL_FILE_HANDLE_H
#define LEAF_FILE_COTROL_FILE_HANDLE_H

#include <queue>

#include "protocol/codec.h"
#include "protocol/message.h"
#include "net/websocket_handle.h"

namespace leaf
{

class cotrol_file_handle : public websocket_handle
{
   public:
    explicit cotrol_file_handle(std::string id, leaf::websocket_session::ptr& session);
    ~cotrol_file_handle() override;

   public:
    void startup() override;
    void shutdown() override;
    std::string type() const override { return "cotrol"; }

   private:
    void on_read(boost::beast::error_code ec, const std::vector<uint8_t>& bytes);
    void on_write(boost::beast::error_code ec, std::size_t bytes_transferred);
    void on_files_request(const std::optional<leaf::files_request>& message);

   private:
    std::string id_;
    leaf::websocket_session::ptr session_;
};

}    // namespace leaf

#endif
