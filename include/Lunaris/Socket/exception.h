#pragma once

#include <stdexcept>

namespace Lunaris {
namespace Socket {

    class SocketException : public std::runtime_error {
    public:
        explicit SocketException(const std::string&) noexcept;
        explicit SocketException(const char*) noexcept;

        const char* what() const noexcept;
    };

} // namespace Socket
} // namespace Lunaris