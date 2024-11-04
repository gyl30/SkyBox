#include <cassert>
#include "codec.h"
#include "reflect.hpp"
#include "net_buffer.h"

#include <type_traits>

namespace reflect
{
REFLECT_STRUCT(leaf::upload_file_request, (file_size)(hash)(filename));
REFLECT_STRUCT(leaf::block_data_finish, (file_id)(hash)(filename));
REFLECT_STRUCT(leaf::create_file_exist, (hash)(filename));
}    // namespace reflect

template <typename E>
constexpr auto to_underlying(E e) noexcept
{
    return static_cast<std::underlying_type_t<E>>(e);
}
namespace leaf
{
uint16_t to_underlying(leaf::message_type type) { return ::to_underlying(type); }
}    // namespace leaf

static void write_padding(leaf::write_buffer &w)
{
    uint64_t xx = 0;       // NOLINT
    w.write_uint64(xx);    // NOLINT
}

namespace leaf
{
void serialize_create_file_request(const upload_file_request &msg, std::vector<uint8_t> *bytes)
{
    leaf::write_buffer w;
    write_padding(w);
    w.write_uint16(leaf::to_underlying(message_type::upload_file_request));
    std::string str = reflect::serialize_struct(const_cast<upload_file_request &>(msg));
    w.write_bytes(str.data(), str.size());
    w.copy_to(bytes);
}

void serialize_create_file_exist(const create_file_exist &msg, std::vector<uint8_t> *bytes)
{
    leaf::write_buffer w;
    write_padding(w);
    w.write_uint16(leaf::to_underlying(message_type::upload_file_exist));
    std::string str = reflect::serialize_struct(const_cast<create_file_exist &>(msg));
    w.write_bytes(str.data(), str.size());
    w.copy_to(bytes);
}

void serialize_create_file_response(const create_file_response &msg, std::vector<uint8_t> *bytes)
{
    leaf::write_buffer w;
    write_padding(w);
    w.write_uint16(leaf::to_underlying(message_type::upload_file_response));
    w.write_uint64(msg.file_id);
    w.write_bytes(msg.filename.data(), msg.filename.size());
    w.copy_to(bytes);
}

void serialize_delete_file_request(const delete_file_request &msg, std::vector<uint8_t> *bytes)
{
    leaf::write_buffer w;
    write_padding(w);
    w.write_uint16(leaf::to_underlying(message_type::delete_file_request));
    w.write_bytes(msg.filename.data(), msg.filename.size());
    w.copy_to(bytes);
}
void serialize_file_block_request(const file_block_request &msg, std::vector<uint8_t> *bytes)
{
    leaf::write_buffer w;
    write_padding(w);
    w.write_uint16(leaf::to_underlying(message_type::file_block_request));
    w.write_uint64(msg.file_id);
    w.copy_to(bytes);
}
void serialize_delete_file_response(const delete_file_response &msg, std::vector<uint8_t> *bytes)
{
    leaf::write_buffer w;
    write_padding(w);
    w.write_uint16(leaf::to_underlying(message_type::delete_file_response));
    w.write_bytes(msg.filename.data(), msg.filename.size());
    w.copy_to(bytes);
}
void serialize_block_data_request(const block_data_request &msg, std::vector<uint8_t> *bytes)
{
    leaf::write_buffer w;
    write_padding(w);
    w.write_uint16(leaf::to_underlying(message_type::block_data_request));
    w.write_uint64(msg.file_id);
    w.write_uint32(msg.block_id);
    w.copy_to(bytes);
}

void serialize_file_block_response(const file_block_response &msg, std::vector<uint8_t> *bytes)
{
    leaf::write_buffer w;
    write_padding(w);
    w.write_uint16(leaf::to_underlying(message_type::file_block_response));
    w.write_uint64(msg.file_id);
    w.write_uint32(msg.block_size);
    w.write_uint32(msg.block_count);
    w.copy_to(bytes);
}
void serialize_block_data_response(const block_data_response &msg, std::vector<uint8_t> *bytes)
{
    leaf::write_buffer w;
    write_padding(w);
    w.write_uint16(leaf::to_underlying(message_type::block_data_response));
    w.write_uint64(msg.file_id);
    w.write_uint32(msg.block_id);
    w.write_bytes(msg.data.data(), msg.data.size());
    w.copy_to(bytes);
}
void serialize_block_data_finish(const block_data_finish &msg, std::vector<uint8_t> *bytes)
{
    leaf::write_buffer w;
    write_padding(w);
    w.write_uint16(leaf::to_underlying(message_type::block_data_finish));
    std::string str = reflect::serialize_struct(const_cast<block_data_finish &>(msg));
    w.write_bytes(str.data(), str.size());
    w.copy_to(bytes);
}

void serialize_error_response(const error_response &msg, std::vector<uint8_t> *bytes)
{
    leaf::write_buffer w;
    write_padding(w);
    w.write_uint16(leaf::to_underlying(message_type::error));
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
            if constexpr (std::is_same_v<T, leaf::upload_file_request>)
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
            else if constexpr (std::is_same_v<T, leaf::block_data_finish>)
            {
                serialize_block_data_finish(arg, bytes);
            }
            else if constexpr (std::is_same_v<T, leaf::create_file_exist>)
            {
                serialize_create_file_exist(arg, bytes);
            }
            else
            {
                ret = -1;
            }
        },
        msg);
    return ret;
}

static std::optional<leaf::upload_file_request> decode_create_file_request(leaf::read_buffer &r)
{
    if (r.size() > 2048)
    {
        return {};
    }

    std::string str;
    r.read_string(&str, r.size());
    if (str.empty())
    {
        return {};
    }
    leaf::upload_file_request req;
    if (!reflect::deserialize_struct(req, str))
    {
        return {};
    }
    return req;
}

static std::optional<leaf::create_file_exist> decode_create_file_exist(leaf::read_buffer &r)
{
    if (r.size() > 2048)
    {
        return {};
    }
    std::string str;
    r.read_string(&str, r.size());
    if (str.empty())
    {
        return {};
    }

    leaf::create_file_exist create;
    if (!reflect::deserialize_struct(create, str))
    {
        return {};
    }
    return create;
}

static std::optional<leaf::delete_file_request> decode_delete_file_request(leaf::read_buffer &r)
{
    if (r.size() > 2048)
    {
        return {};
    }
    std::string filename;
    r.read_string(&filename, r.size());
    if (filename.empty())
    {
        return {};
    }
    leaf::delete_file_request req;
    req.filename = filename;
    return req;
}
static std::optional<leaf::create_file_response> decode_create_file_response(leaf::read_buffer &r)
{
    if (r.size() > 2048)
    {
        return {};
    }
    uint64_t file_id = 0;
    r.read_uint64(&file_id);
    std::string filename;
    r.read_string(&filename, r.size());
    leaf::create_file_response create;
    create.filename = filename;
    create.file_id = file_id;
    return create;
}
static std::optional<leaf::file_block_request> decode_file_block_request(leaf::read_buffer &r)
{
    if (r.size() > 2048)
    {
        return {};
    }
    uint64_t file_id = 0;
    r.read_uint64(&file_id);
    leaf::file_block_request req;
    req.file_id = file_id;
    return req;
}

static std::optional<leaf::file_block_response> decode_file_block_response(leaf::read_buffer &r)
{
    if (r.size() > 2048)
    {
        return {};
    }
    uint64_t file_id = 0;
    r.read_uint64(&file_id);

    uint32_t block_size = 0;
    r.read_uint32(&block_size);

    uint32_t block_count = 0;
    r.read_uint32(&block_count);

    leaf::file_block_response res;
    res.file_id = file_id;
    res.block_count = block_count;
    res.block_size = block_size;
    return res;
}
static std::optional<leaf::block_data_request> decode_block_data_request(leaf::read_buffer &r)
{
    if (r.size() > 2048)
    {
        return {};
    }
    uint64_t file_id = 0;
    r.read_uint64(&file_id);
    uint32_t block_id = 0;
    r.read_uint32(&block_id);

    leaf::block_data_request req;
    req.file_id = file_id;
    req.block_id = block_id;
    return req;
}
static std::optional<leaf::block_data_response> decode_block_data_response(leaf::read_buffer &r)
{
    uint64_t file_id = 0;
    r.read_uint64(&file_id);
    uint32_t block_id = 0;
    r.read_uint32(&block_id);
    std::vector<uint8_t> block_data(r.size(), '0');
    r.read_bytes(block_data.data(), r.size());
    leaf::block_data_response block;
    block.file_id = file_id;
    block.block_id = block_id;
    block.data.swap(block_data);
    return block;
}
static std::optional<leaf::block_data_finish> decode_block_data_finish(leaf::read_buffer &r)
{
    if (r.size() > 2048)
    {
        return {};
    }
    std::string str;
    r.read_string(&str, r.size());
    if (str.empty())
    {
        return {};
    }
    leaf::block_data_finish finish;
    if (!reflect::deserialize_struct(finish, str))
    {
        return {};
    }
    return finish;
}

std::optional<codec_message> deserialize_message(const uint8_t *data, uint64_t len)
{
    leaf::read_buffer r(data, len);
    uint64_t msg_padding = 0;
    r.read_uint64(&msg_padding);
    uint16_t msg_type = 0;
    r.read_uint16(&msg_type);
    if (msg_type == leaf::to_underlying(leaf::message_type::upload_file_request))
    {
        return decode_create_file_request(r);
    }
    if (msg_type == leaf::to_underlying(leaf::message_type::delete_file_request))
    {
        return decode_delete_file_request(r);
    }
    if (msg_type == leaf::to_underlying(leaf::message_type::upload_file_response))
    {
        return decode_create_file_response(r);
    }
    if (msg_type == leaf::to_underlying(leaf::message_type::file_block_request))
    {
        return decode_file_block_request(r);
    }
    if (msg_type == leaf::to_underlying(leaf::message_type::file_block_response))
    {
        return decode_file_block_response(r);
    }
    if (msg_type == leaf::to_underlying(leaf::message_type::block_data_request))
    {
        return decode_block_data_request(r);
    }
    if (msg_type == leaf::to_underlying(leaf::message_type::block_data_response))
    {
        return decode_block_data_response(r);
    }
    if (msg_type == leaf::to_underlying(leaf::message_type::block_data_finish))
    {
        return decode_block_data_finish(r);
    }
    if (msg_type == leaf::to_underlying(leaf::message_type::upload_file_exist))
    {
        return decode_create_file_exist(r);
    }

    return {};
}

}    // namespace leaf
