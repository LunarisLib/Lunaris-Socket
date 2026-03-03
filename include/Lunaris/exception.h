#pragma once

#include <stdexcept>

namespace Lunaris {
namespace Socket {

    class socket_exception : public std::runtime_error {
    public:
        explicit socket_exception(const std::string&) noexcept;
        explicit socket_exception(const char*) noexcept;

        const char* what() const noexcept;
    };

} // namespace Socket
} // namespace Lunaris