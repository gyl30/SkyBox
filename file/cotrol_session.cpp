#include <utility>

#include "file/cotrol_session.h"

namespace leaf
{

cotrol_session::cotrol_session(std::string id, leaf::cotrol_progress_callback cb)
    : id_(std::move(id)), cb_(std::move(cb))
{
}

cotrol_session::~cotrol_session() = default;

void cotrol_session::startup() {}

void cotrol_session::shutdown() {}

}    // namespace leaf
