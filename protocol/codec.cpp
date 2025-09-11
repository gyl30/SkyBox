#include <cassert>
#include <type_traits>
#include "protocol/codec.h"
#include "net/reflect.hpp"
#include "net/net_buffer.h"

namespace reflect
{
REFLECT_STRUCT(leaf::create_dir, (parent)(dir)(token));
REFLECT_STRUCT(leaf::keepalive, (id)(client_id)(client_timestamp)(server_timestamp));
REFLECT_STRUCT(leaf::login_request, (username)(password));
REFLECT_STRUCT(leaf::rename_request, (type)(token)(parent)(old_name)(new_name));
REFLECT_STRUCT(leaf::login_token, (id)(token));
REFLECT_STRUCT(leaf::error_message, (id)(error));
REFLECT_STRUCT(leaf::upload_file_request, (id)(filesize)(dir)(filename));
REFLECT_STRUCT(leaf::upload_file_response, (id)(filename));
REFLECT_STRUCT(leaf::download_file_response, (id)(filesize)(filename)(offset)(hash));
REFLECT_STRUCT(leaf::download_file_request, (id)(filename)(dir)(offset)(hash));
REFLECT_STRUCT(leaf::delete_file_request, (id)(filename));
REFLECT_STRUCT(leaf::file_node, (parent)(name)(type)(file_size));
REFLECT_STRUCT(leaf::files_request, (token)(dir));
REFLECT_STRUCT(leaf::files_response, (files)(token)(dir));
}    // namespace reflect

namespace leaf
{
static uint16_t to_underlying(leaf::message_type type) { return static_cast<std::underlying_type_t<leaf::message_type>>(type); }

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

leaf::message_type get_message_type(std::string_view data)
{
    leaf::read_buffer r(data.data(), data.size());
    read_padding(r);
    uint16_t type = 0;
    r.read_uint16(&type);
    return static_cast<leaf::message_type>(type);
}

leaf::message_type get_message_type(const std::string &data) { return get_message_type(std::string_view(data)); }

leaf::message_type get_message_type(const std::vector<uint8_t> &data)
{
    leaf::read_buffer r(data.data(), data.size());
    read_padding(r);
    uint16_t type = 0;
    r.read_uint16(&type);
    return static_cast<leaf::message_type>(type);
}

std::string message_type_to_string(leaf::message_type type)
{
#define MESSAGE_TYPE_TO_STRING(t)      \
    if (type == leaf::message_type::t) \
    {                                  \
        return #t;                     \
    }
    MESSAGE_TYPE_TO_STRING(keepalive)
    MESSAGE_TYPE_TO_STRING(login)
    MESSAGE_TYPE_TO_STRING(upload_file_request)
    MESSAGE_TYPE_TO_STRING(upload_file_response)
    MESSAGE_TYPE_TO_STRING(delete_file_request)
    MESSAGE_TYPE_TO_STRING(delete_file_response)
    MESSAGE_TYPE_TO_STRING(download_file_request)
    MESSAGE_TYPE_TO_STRING(download_file_response)
    MESSAGE_TYPE_TO_STRING(files_request)
    MESSAGE_TYPE_TO_STRING(files_response)
    MESSAGE_TYPE_TO_STRING(file_data)
    MESSAGE_TYPE_TO_STRING(ack)
    MESSAGE_TYPE_TO_STRING(done)
    MESSAGE_TYPE_TO_STRING(dir)
    MESSAGE_TYPE_TO_STRING(error)
    MESSAGE_TYPE_TO_STRING(rename)
#undef MESSAGE_TYPE_TO_STRING
    return "unknown";
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
std::optional<leaf::keepalive> deserialize_keepalive(const std::vector<uint8_t> &data)
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
    std::string str = reflect::serialize_struct(f);
    w.write_bytes(str.data(), str.size());
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
    if (!reflect::deserialize_struct(f, str))
    {
        return {};
    }
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
    uint32_t hash_size = 0;
    uint32_t data_size = 0;
    r.read_uint32(&hash_size);
    r.read_uint32(&data_size);
    file_data fd;
    if (hash_size > 0)
    {
        fd.hash.assign(hash_size, 0);
        r.read_bytes(fd.hash.data(), hash_size);
    }
    fd.data.assign(data_size, 0);
    r.read_bytes(fd.data.data(), data_size);
    return fd;
}

std::vector<uint8_t> serialize_ack(const ack & /*a*/)
{
    leaf::write_buffer w;
    write_padding(w);
    w.write_uint16(leaf::to_underlying(message_type::ack));
    std::vector<uint8_t> bytes;
    w.copy_to(&bytes);
    return bytes;
}

std::optional<leaf::ack> deserialize_ack(const std::vector<uint8_t> &data)
{
    leaf::read_buffer r(data.data(), data.size());
    read_padding(r);
    uint16_t type = 0;
    r.read_uint16(&type);
    if (type != leaf::to_underlying(message_type::ack))
    {
        return {};
    }
    return leaf::ack{};
}
std::vector<uint8_t> serialize_done(const done & /*d*/)
{
    leaf::write_buffer w;
    write_padding(w);
    w.write_uint16(leaf::to_underlying(message_type::done));
    std::vector<uint8_t> bytes;
    w.copy_to(&bytes);
    return bytes;
}
std::optional<leaf::done> deserialize_done(const std::vector<uint8_t> &data)
{
    leaf::read_buffer r(data.data(), data.size());
    read_padding(r);
    uint16_t type = 0;
    r.read_uint16(&type);
    if (type != leaf::to_underlying(message_type::done))
    {
        return {};
    }
    return leaf::done{};
}
std::vector<uint8_t> serialize_create_dir(const create_dir &c)
{
    leaf::write_buffer w;
    write_padding(w);
    w.write_uint16(leaf::to_underlying(message_type::dir));
    std::string str = reflect::serialize_struct(c);
    w.write_bytes(str.data(), str.size());
    std::vector<uint8_t> bytes;
    w.copy_to(&bytes);
    return bytes;
}
std::optional<leaf::create_dir> deserialize_create_dir(const std::vector<uint8_t> &data)
{
    leaf::read_buffer r(data.data(), data.size());
    read_padding(r);
    uint16_t type = 0;
    r.read_uint16(&type);
    if (type != leaf::to_underlying(message_type::dir))
    {
        return {};
    }
    std::string str;
    r.read_string(&str, r.size());
    if (str.empty())
    {
        return {};
    }
    leaf::create_dir c;
    if (!reflect::deserialize_struct(c, str))
    {
        return {};
    }
    return c;
}
std::vector<uint8_t> serialize_rename_request(const rename_request &r)
{
    leaf::write_buffer w;
    write_padding(w);
    w.write_uint16(leaf::to_underlying(message_type::rename));
    std::string str = reflect::serialize_struct(r);
    w.write_bytes(str.data(), str.size());
    std::vector<uint8_t> bytes;
    w.copy_to(&bytes);
    return bytes;
}
std::optional<leaf::rename_request> deserialize_rename_request(const std::vector<uint8_t> &data)
{
    leaf::read_buffer r(data.data(), data.size());
    read_padding(r);
    uint16_t type = 0;
    r.read_uint16(&type);
    if (type != leaf::to_underlying(message_type::rename))
    {
        return {};
    }
    std::string str;
    r.read_string(&str, r.size());
    if (str.empty())
    {
        return {};
    }
    leaf::rename_request rq;
    if (!reflect::deserialize_struct(rq, str))
    {
        return {};
    }
    return rq;
}

std::vector<uint8_t> serialize_rename_response(const rename_response &r) { return serialize_rename_request(r); }

std::optional<leaf::rename_response> deserialize_rename_response(const std::vector<uint8_t> &data) { return deserialize_rename_request(data); }

}    // namespace leaf
