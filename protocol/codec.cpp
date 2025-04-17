#include <cassert>
#include <type_traits>
#include "protocol/codec.h"
#include "net/reflect.hpp"
#include "net/net_buffer.h"

namespace reflect
{
REFLECT_STRUCT(leaf::keepalive, (id)(client_id)(client_timestamp)(server_timestamp));
REFLECT_STRUCT(leaf::login_request, (username)(password));
REFLECT_STRUCT(leaf::login_token, (id)(token));
REFLECT_STRUCT(leaf::error_message, (id)(error));
REFLECT_STRUCT(leaf::upload_file_request, (id)(block_count)(padding_size)(filename));
REFLECT_STRUCT(leaf::upload_file_response, (id)(filename));
REFLECT_STRUCT(leaf::download_file_response, (id)(block_count)(padding_size)(filename));
REFLECT_STRUCT(leaf::download_file_request, (id)(filename));
REFLECT_STRUCT(leaf::delete_file_request, (id)(filename));
REFLECT_STRUCT(leaf::files_response, (files)(token));
REFLECT_STRUCT(leaf::files_response::file_node, (parent)(name)(type));
}    // namespace reflect

namespace leaf
{
static uint16_t to_underlying(leaf::message_type type)
{
    return static_cast<std::underlying_type_t<leaf::message_type>>(type);
}

static void write_padding(leaf::write_buffer &w)
{
    uint64_t xx = 0;
    w.write_uint64(xx);
}
static void read_padding(leaf::read_buffer &r)
{
    uint64_t xx = 0;
    r.read_uint64(&xx);
}

leaf::message_type get_message_type(const std::string &data)
{
    leaf::read_buffer r(data.data(), data.size());
    read_padding(r);
    uint16_t type = 0;
    r.read_uint16(&type);
    return static_cast<leaf::message_type>(type);
}
leaf::message_type get_message_type(const std::vector<uint8_t> &data)
{
    leaf::read_buffer r(data.data(), data.size());
    read_padding(r);
    uint16_t type = 0;
    r.read_uint16(&type);
    return static_cast<leaf::message_type>(type);
}

std::vector<uint8_t> serialize_upload_file_request(const upload_file_request &msg)
{
    leaf::write_buffer w;
    write_padding(w);
    w.write_uint16(leaf::to_underlying(message_type::upload_file_request));
    std::string str = reflect::serialize_struct(msg);
    w.write_bytes(str.data(), str.size());
    std::vector<uint8_t> bytes;
    w.copy_to(&bytes);
    return bytes;
}
std::optional<leaf::upload_file_request> deserialize_upload_file_request(const std::vector<uint8_t> &data)
{
    leaf::read_buffer r(data.data(), data.size());
    if (r.size() > 2048)
    {
        return {};
    }
    read_padding(r);

    uint16_t type = 0;
    r.read_uint16(&type);
    if (type != leaf::to_underlying(message_type::upload_file_request))
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
// response
std::vector<uint8_t> serialize_upload_file_response(const upload_file_response &msg)
{
    leaf::write_buffer w;
    write_padding(w);
    w.write_uint16(leaf::to_underlying(message_type::upload_file_response));
    std::string str = reflect::serialize_struct(msg);
    w.write_bytes(str.data(), str.size());
    std::vector<uint8_t> bytes;
    w.copy_to(&bytes);
    return bytes;
}
std::optional<leaf::upload_file_response> deserialize_upload_file_response(const std::vector<uint8_t> &data)
{
    leaf::read_buffer r(data.data(), data.size());
    if (r.size() > 2048)
    {
        return {};
    }
    read_padding(r);
    uint16_t type = 0;
    r.read_uint16(&type);
    if (type != leaf::to_underlying(message_type::upload_file_response))
    {
        return {};
    }
    std::string str;
    r.read_string(&str, r.size());
    if (str.empty())
    {
        return {};
    }
    leaf::upload_file_response resp;
    if (!reflect::deserialize_struct(resp, str))
    {
        return {};
    }
    return resp;
}

std::vector<uint8_t> serialize_error_message(const error_message &msg)
{
    leaf::write_buffer w;
    write_padding(w);
    w.write_uint16(leaf::to_underlying(message_type::error));
    std::string str = reflect::serialize_struct(msg);
    w.write_bytes(str.data(), str.size());
    std::vector<uint8_t> bytes;
    w.copy_to(&bytes);
    return bytes;
}

std::optional<leaf::error_message> deserialize_error_message(const std::vector<uint8_t> &data)
{
    leaf::read_buffer r(data.data(), data.size());
    if (r.size() > 2048)
    {
        return {};
    }
    read_padding(r);
    uint16_t type = 0;
    r.read_uint16(&type);
    if (type != leaf::to_underlying(message_type::error))
    {
        return {};
    }
    std::string str;
    r.read_string(&str, r.size());
    if (str.empty())
    {
        return {};
    }
    leaf::error_message req;
    if (!reflect::deserialize_struct(req, str))
    {
        return {};
    }
    return req;
}

std::vector<uint8_t> serialize_download_file_request(const download_file_request &msg)
{
    leaf::write_buffer w;
    write_padding(w);
    w.write_uint16(leaf::to_underlying(message_type::download_file_request));
    std::string str = reflect::serialize_struct(msg);
    w.write_bytes(str.data(), str.size());
    std::vector<uint8_t> bytes;
    w.copy_to(&bytes);
    return bytes;
}
std::optional<leaf::download_file_request> deserialize_download_file_request(const std::vector<uint8_t> &data)
{
    leaf::read_buffer r(data.data(), data.size());
    if (r.size() > 2048)
    {
        return {};
    }
    read_padding(r);
    uint16_t type = 0;
    r.read_uint16(&type);
    if (type != leaf::to_underlying(message_type::download_file_request))
    {
        return {};
    }
    std::string str;
    r.read_string(&str, r.size());
    if (str.empty())
    {
        return {};
    }
    leaf::download_file_request req;
    if (!reflect::deserialize_struct(req, str))
    {
        return {};
    }
    return req;
}

std::vector<uint8_t> serialize_download_file_response(const download_file_response &msg)
{
    leaf::write_buffer w;
    write_padding(w);
    w.write_uint16(leaf::to_underlying(message_type::download_file_response));
    std::string str = reflect::serialize_struct(msg);
    w.write_bytes(str.data(), str.size());
    std::vector<uint8_t> bytes;
    w.copy_to(&bytes);
    return bytes;
}
std::optional<leaf::download_file_response> deserialize_download_file_response(const std::vector<uint8_t> &data)
{
    leaf::read_buffer r(data.data(), data.size());
    if (r.size() > 2048)
    {
        return {};
    }
    read_padding(r);
    uint16_t type = 0;
    r.read_uint16(&type);
    if (type != leaf::to_underlying(message_type::download_file_response))
    {
        return {};
    }
    std::string str;
    r.read_string(&str, r.size());
    if (str.empty())
    {
        return {};
    }
    leaf::download_file_response resp;
    reflect::deserialize_struct(resp, str);
    return resp;
}

std::vector<uint8_t> serialize_delete_file_request(const delete_file_request &msg)
{
    leaf::write_buffer w;
    write_padding(w);
    w.write_uint16(leaf::to_underlying(message_type::delete_file_request));
    std::string str = reflect::serialize_struct(msg);
    w.write_bytes(str.data(), str.size());
    std::vector<uint8_t> bytes;
    w.copy_to(&bytes);
    return bytes;
}

std::optional<leaf::delete_file_request> deserialize_delete_file_request(const std::vector<uint8_t> &data)
{
    leaf::read_buffer r(data.data(), data.size());
    if (r.size() > 2048)
    {
        return {};
    }
    read_padding(r);
    uint16_t type = 0;
    r.read_uint16(&type);
    if (type != leaf::to_underlying(message_type::delete_file_request))
    {
        return {};
    }
    std::string str;
    r.read_string(&str, r.size());
    if (str.empty())
    {
        return {};
    }
    leaf::delete_file_request req;
    if (!reflect::deserialize_struct(req, str))
    {
        return {};
    }
    return req;
}

std::vector<uint8_t> serialize_keepalive(const leaf::keepalive &k)
{
    leaf::write_buffer w;
    write_padding(w);
    w.write_uint16(leaf::to_underlying(message_type::keepalive));
    std::string str = reflect::serialize_struct(k);
    w.write_bytes(str.data(), str.size());
    std::vector<uint8_t> bytes;
    w.copy_to(&bytes);
    return bytes;
}
std::optional<leaf::keepalive> deserialize_keepalive_response(const std::vector<uint8_t> &data)
{
    leaf::read_buffer r(data.data(), data.size());
    if (r.size() > 2048)
    {
        return {};
    }
    read_padding(r);
    uint16_t type = 0;
    r.read_uint16(&type);
    if (type != leaf::to_underlying(message_type::keepalive))
    {
        return {};
    }
    std::string str;
    r.read_string(&str, r.size());
    if (str.empty())
    {
        return {};
    }
    leaf::keepalive resp;
    if (!reflect::deserialize_struct(resp, str))
    {
        return {};
    }
    return resp;
}
std::vector<uint8_t> serialize_login_request(const leaf::login_request &l)
{
    leaf::write_buffer w;
    write_padding(w);
    w.write_uint16(leaf::to_underlying(message_type::login));
    std::string str = reflect::serialize_struct(l);
    w.write_bytes(str.data(), str.size());
    std::vector<uint8_t> bytes;
    w.copy_to(&bytes);
    return bytes;
}
std::optional<leaf::login_request> deserialize_login_request(const std::vector<uint8_t> &data)
{
    leaf::read_buffer r(data.data(), data.size());
    if (r.size() > 2048)
    {
        return {};
    }

    read_padding(r);
    uint16_t type = 0;
    r.read_uint16(&type);
    if (type != leaf::to_underlying(message_type::login))
    {
        return {};
    }

    std::string str;
    r.read_string(&str, r.size());
    if (str.empty())
    {
        return {};
    }
    leaf::login_request l;
    if (!reflect::deserialize_struct(l, str))
    {
        return {};
    }
    return l;
}

std::vector<uint8_t> serialize_login_token(const leaf::login_token &l)
{
    leaf::write_buffer w;
    write_padding(w);
    w.write_uint16(leaf::to_underlying(message_type::login));
    std::string str = reflect::serialize_struct(l);
    w.write_bytes(str.data(), str.size());
    std::vector<uint8_t> bytes;
    w.copy_to(&bytes);
    return bytes;
}

std::optional<leaf::login_token> deserialize_login_token(const std::vector<uint8_t> &data)
{
    leaf::read_buffer r(data.data(), data.size());
    if (r.size() > 2048)
    {
        return {};
    }

    read_padding(r);
    uint16_t type = 0;
    r.read_uint16(&type);
    if (type != leaf::to_underlying(message_type::login))
    {
        return {};
    }

    std::string str;
    r.read_string(&str, r.size());
    if (str.empty())
    {
        return {};
    }
    leaf::login_token l;
    if (!reflect::deserialize_struct(l, str))
    {
        return {};
    }
    return l;
}

std::vector<uint8_t> serialize_files_request(const leaf::files_request &f)
{
    leaf::write_buffer w;
    write_padding(w);
    w.write_uint16(leaf::to_underlying(message_type::files_request));
    w.write_bytes(f.token.data(), f.token.size());
    std::vector<uint8_t> bytes;
    w.copy_to(&bytes);
    return bytes;
}

std::optional<leaf::files_request> deserialize_files_request(const std::vector<uint8_t> &data)
{
    leaf::read_buffer r(data.data(), data.size());
    if (r.size() > 2048)
    {
        return {};
    }
    read_padding(r);
    uint16_t type = 0;
    r.read_uint16(&type);
    if (type != leaf::to_underlying(message_type::files_request))
    {
        return {};
    }
    std::string str;
    r.read_string(&str, r.size());
    if (str.empty())
    {
        return {};
    }
    leaf::files_request f;
    f.token = str;
    return f;
}

std::vector<uint8_t> serialize_files_response(const leaf::files_response &f)
{
    leaf::write_buffer w;
    write_padding(w);
    w.write_uint16(leaf::to_underlying(message_type::files_response));
    std::string str = reflect::serialize_struct(f);
    w.write_bytes(str.data(), str.size());
    std::vector<uint8_t> bytes;
    w.copy_to(&bytes);
    return bytes;
}

std::optional<leaf::files_response> deserialize_files_response(const std::vector<uint8_t> &data)
{
    leaf::read_buffer r(data.data(), data.size());
    if (r.size() > 2048)
    {
        return {};
    }
    read_padding(r);
    uint16_t type = 0;
    r.read_uint16(&type);
    if (type != leaf::to_underlying(message_type::files_response))
    {
        return {};
    }
    std::string str;
    r.read_string(&str, r.size());
    if (str.empty())
    {
        return {};
    }
    leaf::files_response f;
    if (!reflect::deserialize_struct(f, str))
    {
        return {};
    }
    return f;
}

std::vector<uint8_t> serialize_file_data(const file_data &data)
{
    leaf::write_buffer w;
    write_padding(w);
    w.write_uint16(leaf::to_underlying(message_type::file_data));
    w.write_uint32(data.block_id);
    uint32_t data_size = data.data.size();
    uint32_t hash_size = data.hash.size();
    w.write_uint32(hash_size);
    w.write_uint32(data_size);
    if (hash_size > 0)
    {
        w.write_bytes(data.hash.data(), hash_size);
    }
    w.write_bytes(data.data);
    std::vector<uint8_t> bytes;
    w.copy_to(&bytes);
    return bytes;
}

std::optional<file_data> deserialize_file_data(const std::vector<uint8_t> &data)
{
    leaf::read_buffer r(data.data(), data.size());
    read_padding(r);
    uint16_t type = 0;
    r.read_uint16(&type);
    if (type != leaf::to_underlying(message_type::file_data))
    {
        return {};
    }
    uint32_t block_id = 0;
    r.read_uint32(&block_id);
    uint32_t hash_size = 0;
    uint32_t data_size = 0;
    r.read_uint32(&hash_size);
    r.read_uint32(&data_size);
    file_data fd;
    fd.block_id = block_id;
    if (hash_size > 0)
    {
        fd.hash.assign(hash_size, 0);
        r.read_bytes(fd.hash.data(), hash_size);
    }
    fd.data.assign(data_size, 0);
    r.read_bytes(fd.data.data(), data_size);
    return fd;
}
}    // namespace leaf
