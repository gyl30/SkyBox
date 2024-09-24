#include "tcp_server.h"
#include "application.h"

namespace leaf
{

application::application(int argc, char** argv) : argc_(argc), argv_(argv) {}

application::~application() = default;

void application::exec() {}

}    // namespace leaf
