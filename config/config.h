#ifndef LEAF_CONFIG_H
#define LEAF_CONFIG_H

#include <filesystem>

namespace leaf
{
constexpr auto kBlockSize = 128 * 1024;
constexpr auto kHashBlockSize = 10 * kBlockSize;
static auto kDefaultDir = std::filesystem::temp_directory_path().string();
constexpr auto kKB = 1024ULL;
constexpr auto kMB = 1024 * kKB;
constexpr auto kReadWsLimited = 1 * kMB;
constexpr auto kWriteWsLimited = 1 * kMB;
constexpr auto kTmpFilenameSuffix = ".tmp";
constexpr auto kLeafFilenameSuffix = ".leaf";

}    // namespace leaf

#endif
