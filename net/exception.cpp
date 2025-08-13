#include <string>
#include <exception>

#include "log/log.h"
#include "net/exception.h"

namespace leaf
{

void cache_exception(const std::string& msg, const std::exception_ptr& eptr)
{
    try
    {
        if (eptr)
        {
            rethrow_exception(eptr);
        }
    }
    catch (const std::exception& e)
    {
        LOG_ERROR("{} {}", msg, e.what());
    }
}

}    // namespace leaf
