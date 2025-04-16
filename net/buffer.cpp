#include <boost/beast/core/buffers_range.hpp>

#include "net/buffer.h"

namespace leaf
{

std::vector<uint8_t> buffers_to_vector(const boost::asio::mutable_buffer& buffers)
{
    auto result = std::vector<uint8_t>();
    result.reserve(boost::asio::buffer_size(buffers));
    for (const auto buff : boost::beast::buffers_range(buffers))
    {
        const auto* data = static_cast<const uint8_t*>(buff.data());
        result.insert(result.end(), data, data + buff.size());
    }
    return result;
}

}    // namespace leaf
