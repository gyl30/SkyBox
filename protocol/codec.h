#ifndef LEAF_PROTOCOL_CODEC_H
#define LEAF_PROTOCOL_CODEC_H

#include <variant>
#include <optional>
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
                                   leaf::error_response>;

uint16_t to_underlying(leaf::message_type type);
std::vector<uint8_t> serialize_message(const codec_message &msg);
std::optional<codec_message> deserialize_message(const uint8_t *data, uint64_t len);

}    // namespace leaf

#endif
