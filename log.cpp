#include <string>
#include <vector>
#include <memory>
#include "log.h"
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>

static uint32_t log_file_size()
{
    constexpr auto kFileSize = 50 * 1024 * 1024;
    return kFileSize;
}
static uint32_t log_file_count()
{
    constexpr auto kFileCount = 5;
    return kFileCount;
}

namespace leaf
{
void init_log(const std::string& filename)
{
    uint32_t file_size = log_file_size();
    uint32_t file_count = log_file_count();
    std::vector<spdlog::sink_ptr> sinks;
    sinks.push_back(std::make_shared<spdlog::sinks::stdout_sink_mt>());
    sinks.push_back(std::make_shared<spdlog::sinks::rotating_file_sink_mt>(filename, file_size, file_count));
    auto logger = std::make_shared<spdlog::logger>("", begin(sinks), end(sinks));
    spdlog::set_default_logger(logger);
    spdlog::flush_every(std::chrono::seconds(3));
    spdlog::set_level(spdlog::level::info);
    spdlog::set_pattern("%Y%m%d %T.%f %t %L %v %s:%#");
}
void set_log_level(const std::string& level)
{
    if (level == "debug")
    {
        spdlog::set_level(spdlog::level::debug);
    }
    else if (level == "warn" || level == "warning")
    {
        spdlog::set_level(spdlog::level::warn);
    }
    else if (level == "err" || level == "error")
    {
        spdlog::set_level(spdlog::level::err);
    }
    else if (level == "trace")
    {
        spdlog::set_level(spdlog::level::trace);
    }
    else
    {
        spdlog::set_level(spdlog::level::info);
    }
}
void shutdown_log()
{
    spdlog::default_logger()->flush();
    spdlog::shutdown();
}
}    // namespace leaf
