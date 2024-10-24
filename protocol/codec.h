#ifndef LEAF_CODEC_H
#define LEAF_CODEC_H

#include <queue>
#include <variant>
#include <functional>
#include "message.h"

namespace leaf
{
struct codec_handle
{
    std::function<void(const leaf::create_file_request &)> create_file_request;
    std::function<void(const leaf::create_file_response &)> create_file_response;
    std::function<void(const leaf::delete_file_request &)> delete_file_request;
    std::function<void(const leaf::file_block_request &)> file_block_request;
    std::function<void(const leaf::delete_file_response &)> delete_file_response;
    std::function<void(const leaf::block_data_request &)> block_data_request;
    std::function<void(const leaf::block_data_response &)> block_data_response;
    std::function<void(const leaf::file_block_response &)> file_block_response;
    std::function<void(const leaf::block_data_finish &)> block_data_finish;
    std::function<void(const leaf::error_response &)> error_response;
    std::function<void(const leaf::create_file_exist &)> create_file_exist;

    std::queue<std::vector<uint8_t>> msg_queue;
};

using codec_message = std::variant<leaf::create_file_request,
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

int serialize_message(const codec_message &msg, std::vector<uint8_t> *bytes);
int deserialize_message(const uint8_t *data, uint64_t len, codec_handle *handle);

}    // namespace leaf

#endif
