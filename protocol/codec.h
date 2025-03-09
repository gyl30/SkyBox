#ifndef LEAF_PROTOCOL_CODEC_H
#define LEAF_PROTOCOL_CODEC_H

#include <variant>
#include <optional>
#include "net/net_buffer.h"
#include "protocol/message.h"

namespace leaf
{
using codec_message = std::variant<leaf::upload_file_request,
                                   leaf::upload_file_response,
                                   leaf::upload_file_exist,
                                   //
                                   leaf::download_file_request,
                                   leaf::download_file_response,
                                   //
                                   leaf::delete_file_request,
                                   leaf::delete_file_response,
                                   //
                                   leaf::file_block_request,
                                   leaf::file_block_response,
                                   //
                                   leaf::block_data_request,
                                   leaf::block_data_response,
                                   leaf::block_data_finish,
                                   //
                                   leaf::login_request,
                                   leaf::login_response,
                                   //
                                   leaf::keepalive,
                                   leaf::error_response,
                                   //
                                   leaf::files_request,
                                   leaf::files_response>;

namespace message
{
std::vector<uint8_t> serialize_upload_file_request(const upload_file_request &msg);
std::optional<leaf::upload_file_request> deserialize_upload_file_request(leaf::read_buffer &r);
std::vector<uint8_t> serialize_upload_file_response(const upload_file_response &msg);
std::optional<leaf::upload_file_response> deserialize_upload_file_response(leaf::read_buffer &r);
std::vector<uint8_t> serialize_upload_file_exist(const upload_file_exist &msg);
std::optional<leaf::upload_file_exist> deserialize_upload_file_exist(leaf::read_buffer &r);
std::vector<uint8_t> serialize_download_file_request(const download_file_request &msg);
std::optional<leaf::download_file_request> deserialize_download_file_request(leaf::read_buffer &r);
std::vector<uint8_t> serialize_download_file_response(const download_file_response &msg);
std::optional<leaf::download_file_response> deserialize_download_file_response(leaf::read_buffer &r);
std::vector<uint8_t> serialize_delete_file_request(const delete_file_request &msg);
std::optional<leaf::delete_file_request> deserialize_delete_file_request(leaf::read_buffer &r);
std::vector<uint8_t> serialize_delete_file_response(const delete_file_response &msg);
std::optional<leaf::delete_file_response> deserialize_delete_file_response(leaf::read_buffer &r);
std::vector<uint8_t> serialize_file_block_request(const file_block_request &msg);
std::optional<leaf::file_block_request> deserialize_file_block_request(leaf::read_buffer &r);
std::vector<uint8_t> serialize_file_block_response(const file_block_response &msg);
std::optional<leaf::file_block_response> deserialize_file_block_response(leaf::read_buffer &r);
std::vector<uint8_t> serialize_block_data_request(const block_data_request &msg);
std::optional<leaf::block_data_request> deserialize_block_data_request(leaf::read_buffer &r);
std::vector<uint8_t> serialize_block_data_response(const block_data_response &msg);
std::optional<leaf::block_data_response> deserialize_block_data_response(leaf::read_buffer &r);
std::vector<uint8_t> serialize_block_data_finish(const block_data_finish &msg);
std::optional<leaf::block_data_finish> deserialize_block_data_finish(leaf::read_buffer &r);
std::vector<uint8_t> serialize_error_response(const error_response &msg);
std::optional<leaf::error_response> deserialize_error_response(leaf::read_buffer &r);
std::vector<uint8_t> serialize_keepalive(const leaf::keepalive &k);
std::optional<leaf::keepalive> deserialize_keepalive_response(leaf::read_buffer &r);
std::vector<uint8_t> serialize_login_request(const leaf::login_request &l);
std::optional<leaf::login_request> deserialize_login_request(leaf::read_buffer &r);
std::vector<uint8_t> serialize_login_response(const leaf::login_response &l);
std::optional<leaf::login_response> deserialize_login_response(leaf::read_buffer &r);
std::vector<uint8_t> serialize_files_request(const leaf::files_request &f);
std::optional<leaf::files_request> deserialize_files_request(leaf::read_buffer &r);
std::vector<uint8_t> serialize_files_response(const leaf::files_response &f);
std::optional<leaf::files_response> deserialize_files_response(leaf::read_buffer &r);
}    // namespace message
uint16_t to_underlying(leaf::message_type type);
std::vector<uint8_t> serialize_message(const codec_message &msg);
std::string serialize_message(const leaf::login_token &l);
std::optional<codec_message> deserialize_message(const uint8_t *data, uint64_t len);
std::optional<leaf::login_token> deserialize_message(const std::string &data);

}    // namespace leaf

#endif
