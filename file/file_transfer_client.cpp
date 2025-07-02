#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/asio/redirect_error.hpp>
#include "log/log.h"
#include "crypt/random.h"
#include "protocol/message.h"
#include "file/file_transfer_client.h"

#include <utility>

namespace leaf
{

file_transfer_client::file_transfer_client(std::string ip, uint16_t port, std::string username, std::string password, std::string token)
    : id_(leaf::random_string(8)),
      host_(std::move(ip)),
      port_(std::to_string(port)),
      token_(std::move(token)),
      user_(std::move(username)),
      pass_(std::move(password))
{
    LOG_INFO("{} created", id_);
}

file_transfer_client::~file_transfer_client() { LOG_INFO("{} destroyed", id_); }

void file_transfer_client::startup()
{
    executors.startup();
    ex_ = &executors.get_executor();
    auto self = shared_from_this();
    cotrol_ = std::make_shared<leaf::cotrol_session>(id_, host_, port_, token_, executors.get_executor());
    upload_ = std::make_shared<leaf::upload_session>(id_, host_, port_, token_, executors.get_executor());
    download_ = std::make_shared<leaf::download_session>(id_, host_, port_, token_, executors.get_executor());
    cotrol_->startup();
    upload_->startup();
    download_->startup();
}

void file_transfer_client::shutdown()
{
    auto self = shared_from_this();
    std::call_once(shutdown_flag_,
                   [this, self]()
                   {
                       boost::asio::co_spawn(
                           *ex_, [this, self]() -> boost::asio::awaitable<void> { co_await shutdown_coro(); }, boost::asio::detached);
                   });
}

boost::asio::awaitable<void> file_transfer_client::shutdown_coro()
{
    auto self = shared_from_this();
    LOG_INFO("{} shutdown", id_);
    if (upload_)
    {
        upload_->shutdown();
        upload_.reset();
    }
    if (download_)
    {
        download_->shutdown();
        download_.reset();
    }
    if (cotrol_)
    {
        cotrol_->shutdown();
        cotrol_.reset();
    }
    boost::system::error_code ec;
    boost::asio::steady_timer timer(co_await boost::asio::this_coro::executor, std::chrono::seconds(1));
    co_await timer.async_wait(boost::asio::redirect_error(boost::asio::use_awaitable, ec));
    timer.cancel();
    LOG_INFO("{} shutdown complete", id_);
}

void file_transfer_client::add_upload_file(leaf::file_info f)
{
    boost::asio::post(*ex_,
                      [this, f = std::move(f)]()
                      {
                          if (upload_)
                          {
                              upload_->add_file(f);
                          }
                      });
}
void file_transfer_client::add_download_file(leaf::file_info f)
{
    boost::asio::post(*ex_,
                      [this, f = std::move(f)]()
                      {
                          if (download_)
                          {
                              download_->add_file(f);
                          }
                      });
}
void file_transfer_client::add_upload_files(const std::vector<leaf::file_info> &files)
{
    boost::asio::post(*ex_,
                      [this, files]()
                      {
                          if (upload_)
                          {
                              upload_->add_files(files);
                          }
                      });
}
void file_transfer_client::add_download_files(const std::vector<file_info> &files)
{
    boost::asio::post(*ex_,
                      [this, files]()
                      {
                          if (download_)
                          {
                              download_->add_files(files);
                          }
                      });
}

void file_transfer_client::create_directory(const std::string &dir)
{
    if (cotrol_)
    {
        cotrol_->create_directory(dir);
    }
}
void file_transfer_client::change_current_dir(const std::string &dir)
{
    if (cotrol_)
    {
        cotrol_->change_current_dir(dir);
    }
}
}    // namespace leaf
