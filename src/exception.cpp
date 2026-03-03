#include <Lunaris/exception.h>


namespace Lunaris {
namespace Socket {

    socket_exception::socket_exception(const std::string& msg) noexcept
        : std::runtime_error(msg)
    {
    }

    socket_exception::socket_exception(const char* msg) noexcept
        : std::runtime_error(msg)
    {
    }

    const char* socket_exception::what() const noexcept
    {
        return std::runtime_error::what();
    }

} // namespace Socket
} // namespace Lunaris