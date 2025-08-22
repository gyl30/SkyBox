#include <vector>
#include <boost/asio.hpp>
#include <boost/system/error_code.hpp>

#include "file/file.h"
#include "crypt/blake2b.h"
#include "file/hash_file.h"

namespace leaf
{

std::string hash_file(const std::string& file, boost::system::error_code& ec, std::size_t read_limit)
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
    std::size_t read_size = 0;
    while (read_size < read_limit)
    {
        auto r_size = std::min<std::size_t>(read_limit - read_size, kBufferSize);
        auto len = f.read(buffer.data(), r_size, ec);

        if (ec == boost::asio::error::eof)
        {
            ec = {};
            if (len > 0)
            {
                b.update(buffer.data(), len);
            }
            break;
        }
        if (ec)
        {
            break;
        }
        b.update(buffer.data(), len);
        read_size += len;
    }
    b.final();
    auto close_ec = f.close();
    if (ec)
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
