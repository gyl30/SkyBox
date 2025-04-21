#ifndef LEAF_CONFIG_H
#define LEAF_CONFIG_H

namespace leaf
{
constexpr auto kBlockSize = 128 * 1024;
constexpr auto kHashBlockCount = 10;
constexpr auto kDefatuleDownloadDir = "tmp";
constexpr auto kReadWsLimited = 256 * 1024;
constexpr auto kWriteWsLimited = 256 * 1024;

}    // namespace leaf

#endif
