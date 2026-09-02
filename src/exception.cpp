#include <Lunaris/Socket/exception.h>


namespace Lunaris {
namespace Socket {

    SocketException::SocketException(const std::string& msg) noexcept
        : std::runtime_error(msg)
    {
    }

    SocketException::SocketException(const char* msg) noexcept
        : std::runtime_error(msg)
    {
    }

    const char* SocketException::what() const noexcept {
        return std::runtime_error::what();
    }

} // namespace Socket
} // namespace Lunaris