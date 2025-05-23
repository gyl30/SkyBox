#ifndef LEAF_CONFIG_H
#define LEAF_CONFIG_H

namespace leaf
{
constexpr auto kBlockSize = 128 * 1024;
constexpr auto kHashBlockCount = 10;
constexpr auto kDefaultDir = "/tmp";
constexpr auto kReadWsLimited = 2 * 1024 * 1024;
constexpr auto kWriteWsLimited = 2 * 1024 * 1024;
constexpr auto kTmpFilenameSuffix = ".tmp";
constexpr auto kLeafFilenameSuffix = ".leaf";

}    // namespace leaf

#endif
