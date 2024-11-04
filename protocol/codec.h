#ifndef LEAF_CODEC_H
#define LEAF_CODEC_H

#include <variant>
#include <optional>
#include "message.h"

namespace leaf
{
using codec_message = std::variant<leaf::upload_file_request,
                                   leaf::create_file_exist,
                                   leaf::create_file_response,
                                   leaf::delete_file_request,
                                   leaf::file_block_request,
                                   leaf::delete_file_response,
                                   leaf::block_data_request,
                                   leaf::block_data_response,
                                   leaf::file_block_response,
                                   leaf::block_data_finish,
                                   leaf::error_response>;

uint16_t to_underlying(leaf::message_type type);
int serialize_message(const codec_message &msg, std::vector<uint8_t> *bytes);
std::optional<codec_message> deserialize_message(const uint8_t *data, uint64_t len);

}    // namespace leaf

#endif
