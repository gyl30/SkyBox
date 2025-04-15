#ifndef LEAF_PROTOCOL_CODEC_H
#define LEAF_PROTOCOL_CODEC_H

#include <optional>
#include "protocol/message.h"

namespace leaf
{
leaf::message_type get_message_type(const std::string &data);
leaf::message_type get_message_type(const std::vector<uint8_t> &data);
std::vector<uint8_t> serialize_keepalive(const leaf::keepalive &k);
std::vector<uint8_t> serialize_error_message(const error_message &msg);
std::vector<uint8_t> serialize_login_request(const leaf::login_request &l);
std::vector<uint8_t> serialize_login_token(const leaf::login_token &l);
std::vector<uint8_t> serialize_files_request(const leaf::files_request &f);
std::vector<uint8_t> serialize_files_response(const leaf::files_response &f);
std::vector<uint8_t> serialize_upload_file_request(const upload_file_request &msg);
std::vector<uint8_t> serialize_download_file_request(const download_file_request &msg);
std::vector<uint8_t> serialize_download_file_response(const download_file_response &msg);
std::vector<uint8_t> serialize_delete_file_request(const delete_file_request &msg);
std::vector<uint8_t> serialize_file_data(const file_data &data);

std::optional<leaf::error_message> deserialize_error_message(const std::vector<uint8_t> &data);
std::optional<leaf::upload_file_request> deserialize_upload_file_request(const std::vector<uint8_t> &data);
std::optional<leaf::download_file_request> deserialize_download_file_request(const std::vector<uint8_t> &data);
std::optional<leaf::download_file_response> deserialize_download_file_response(const std::vector<uint8_t> &data);
std::optional<leaf::delete_file_request> deserialize_delete_file_request(const std::vector<uint8_t> &data);
std::optional<leaf::keepalive> deserialize_keepalive_response(const std::vector<uint8_t> &data);
std::optional<leaf::login_request> deserialize_login_request(const std::vector<uint8_t> &data);
std::optional<leaf::login_token> deserialize_login_token(const std::vector<uint8_t> &data);
std::optional<leaf::files_request> deserialize_files_request(const std::vector<uint8_t> &data);
std::optional<leaf::files_response> deserialize_files_response(const std::vector<uint8_t> &data);
std::optional<leaf::file_data> deserialize_file_data(const std::vector<uint8_t> &data);

}    // namespace leaf

#endif
