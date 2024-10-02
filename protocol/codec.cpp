#include "codec.h"
#include "net_buffer.h"

#include <type_traits>

template <typename E>
constexpr auto to_underlying(E e) noexcept
{
    return static_cast<std::underlying_type_t<E>>(e);
}

static void write_padding(leaf::write_buffer &w)
{
    uint64_t xx;           // NOLINT
    w.write_uint64(xx);    // NOLINT
}

namespace leaf
{
void serialize_create_file_request(const create_file_request &msg, std::vector<uint8_t> *bytes)
{
    leaf::write_buffer w;
    write_padding(w);
    w.write_uint16(to_underlying(message_type::create_file_request));
    w.write_uint64(msg.file_size);
    w.write_bytes(msg.filename.data(), msg.filename.size());
    w.copy_to(bytes);
}
void serialize_create_file_response(const create_file_response &msg, std::vector<uint8_t> *bytes)
{
    leaf::write_buffer w;
    write_padding(w);
    w.write_uint16(to_underlying(message_type::create_file_response));
    w.write_uint64(msg.file_id);
    w.write_bytes(msg.filename.data(), msg.filename.size());
    w.copy_to(bytes);
}

void serialize_delete_file_request(const delete_file_request &msg, std::vector<uint8_t> *bytes)
{
    leaf::write_buffer w;
    write_padding(w);
    w.write_uint16(to_underlying(message_type::delete_file_request));
    w.write_bytes(msg.filename.data(), msg.filename.size());
    w.copy_to(bytes);
}
void serialize_file_block_request(const file_block_request &msg, std::vector<uint8_t> *bytes)
{
    leaf::write_buffer w;
    write_padding(w);
    w.write_uint16(to_underlying(message_type::file_block_request));
    w.write_uint64(msg.file_id);
    w.copy_to(bytes);
}
void serialize_delete_file_response(const delete_file_response &msg, std::vector<uint8_t> *bytes)
{
    leaf::write_buffer w;
    write_padding(w);
    w.write_uint16(to_underlying(message_type::delete_file_response));
    w.write_bytes(msg.filename.data(), msg.filename.size());
    w.copy_to(bytes);
}
void serialize_block_data_request(const block_data_request &msg, std::vector<uint8_t> *bytes)
{
    leaf::write_buffer w;
    write_padding(w);
    w.write_uint16(to_underlying(message_type::block_data_request));
    w.write_uint32(msg.block_id);
    w.write_uint64(msg.file_id);
    w.copy_to(bytes);
}

void serialize_file_block_response(const file_block_response &msg, std::vector<uint8_t> *bytes)
{
    leaf::write_buffer w;
    write_padding(w);
    w.write_uint16(to_underlying(message_type::file_block_response));
    w.write_uint32(msg.block_size);
    for (auto block : msg.blocks)
    {
        w.write_uint32(block);
    }
    w.copy_to(bytes);
}
void serialize_block_data_response(const block_data_response &msg, std::vector<uint8_t> *bytes)
{
    leaf::write_buffer w;
    write_padding(w);
    w.write_uint16(to_underlying(message_type::block_data_response));
    w.write_uint64(msg.file_id);
    w.write_uint32(msg.block_id);
    w.write_bytes(msg.data.data(), msg.data.size());
    w.copy_to(bytes);
}

void serialize_error_response(const error_response &msg, std::vector<uint8_t> *bytes)
{
    leaf::write_buffer w;
    write_padding(w);
    w.write_uint16(to_underlying(message_type::error));
    w.write_uint32(msg.error);
    w.copy_to(bytes);
}
int serialize_message(const codec_message &msg, std::vector<uint8_t> *bytes)
{
    //

    int ret = 0;
    std::visit(
        [&](auto &&arg)
        {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, leaf::create_file_request>)
            {
                serialize_create_file_request(arg, bytes);
            }
            else if constexpr (std::is_same_v<T, leaf::delete_file_request>)
            {
                serialize_delete_file_request(arg, bytes);
            }
            else if constexpr (std::is_same_v<T, leaf::file_block_request>)
            {
                serialize_file_block_request(arg, bytes);
            }
            else if constexpr (std::is_same_v<T, leaf::block_data_request>)
            {
                serialize_block_data_request(arg, bytes);
            }

            else if constexpr (std::is_same_v<T, leaf::create_file_response>)
            {
                serialize_create_file_response(arg, bytes);
            }
            else if constexpr (std::is_same_v<T, leaf::delete_file_response>)
            {
                serialize_delete_file_response(arg, bytes);
            }
            else if constexpr (std::is_same_v<T, leaf::file_block_response>)
            {
                serialize_file_block_response(arg, bytes);
            }
            else if constexpr (std::is_same_v<T, leaf::block_data_response>)
            {
                serialize_block_data_response(arg, bytes);
            }
            else if constexpr (std::is_same_v<T, leaf::error_response>)
            {
                serialize_error_response(arg, bytes);
            }
            else
            {
                ret = -1;
            }
        },
        msg);
    return ret;
}

static int decode_create_file_request(leaf::read_buffer &r, codec_handle *handle)
{
    if (r.size() > 2048)
    {
        return -1;
    }

    //  max size 10G
    constexpr auto max_file_size = 1024L * 1024 * 1024 * 10;
    uint64_t file_size = 0;
    r.read_uint64(&file_size);
    if (file_size == 0 || file_size > max_file_size)
    {
        return -1;
    }
    std::string filename;
    r.read_string(&filename, r.size());
    if (filename.empty())
    {
        return -1;
    }
    leaf::create_file_request req;
    req.file_size = file_size;
    req.filename = filename;
    handle->create_file_request(req);
    return 0;
}
static int decode_delete_file_request(leaf::read_buffer &r, codec_handle *handle)
{
    if (r.size() > 2048)
    {
        return -1;
    }
    std::string filename;
    r.read_string(&filename, r.size());
    if (filename.empty())
    {
        return -1;
    }
    leaf::delete_file_request req;
    req.filename = filename;
    handle->delete_file_request(req);
    return 0;
}

int deserialize_message(const uint8_t *data, uint64_t len, codec_handle *handle)
{
    leaf::read_buffer r(data, len);
    uint32_t msg_padding = 0;
    r.read_uint32(&msg_padding);
    uint16_t msg_type = 0;
    r.read_uint16(&msg_type);
    if (msg_type == to_underlying(leaf::message_type::create_file_request))
    {
        return decode_create_file_request(r, handle);
    }
    if (msg_type == to_underlying(leaf::message_type::delete_file_request))
    {
        return decode_delete_file_request(r, handle);
    }

    return -1;
}

}    // namespace leaf
