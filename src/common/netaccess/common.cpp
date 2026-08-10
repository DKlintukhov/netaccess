#include <netaccess/common.h>

#include <netaccess/version.h>

namespace netaccess
{

const char* versionString() noexcept
{
    return NETACCESS_VERSION_STRING;
}

} // namespace netaccess
