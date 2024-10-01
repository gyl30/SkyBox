#include "websocket_handle.h"
namespace leaf
{

class file_websocket_handle : public websocket_handle
{
   public:
    explicit file_websocket_handle(std::string id);
    ~file_websocket_handle() override;

   public:
    void startup() override;
    void on_text_message(const std::string& msg) override;
    void on_binary_message(const std::string& bin) override;
    void shutdown() override;

   private:
    std::string id_;
};

}    // namespace leaf
