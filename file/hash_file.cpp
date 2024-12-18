#include <vector>
#include <boost/asio.hpp>
#include <boost/system/error_code.hpp>

#include "file/file.h"
#include "file/hash_file.h"
#include "crypt/blake2b.h"

namespace leaf
{

std::string hash_file(const std::string& file, boost::system::error_code& ec)
{
    leaf::file_reader f(file);
    ec = f.open();
    if (ec)
    {
        return {};
    }
    leaf::blake2b b;
    constexpr auto kBufferSize = 4096;
    std::vector<uint8_t> buffer(kBufferSize, '0');
    while (true)
    {
        auto read_size = f.read(buffer.data(), buffer.size(), ec);
        if (ec)
        {
            break;
        }
        b.update(buffer.data(), read_size);
    }
    auto close_ec = f.close();
    if (ec == boost::asio::error::eof)
    {
        ec = {};
        b.final();
    }
    else if (ec)
    {
        return {};
    }
    if (close_ec)
    {
        return {};
    }
    return b.hex();
}
}    // namespace leaf
