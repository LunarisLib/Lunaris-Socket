#include <source_location>
#include <print>
#include <format>
#include <string.h>
#include <mutex>

namespace Lunaris {
namespace Socket {

    template <typename... Args>
    inline void __lunaris_socket_debug(
        std::format_string<Args...> fmt,
        const std::source_location& loc,
        Args&&... args        
    ) {
#ifdef LUNARIS_DEBUG
        static std::mutex mtx;

        constexpr size_t max_len = 20;
        constexpr size_t max_fcn_len = 60;

        const auto filename = loc.file_name();
        const auto filename_len = strlen(filename);

        const auto funname = loc.function_name();
        const auto funname_len = strlen(funname);

        std::unique_lock<std::mutex> m(mtx);
        std::printf("[ %*s : %04u ] %*s $ ",
            max_len, 
            filename_len > max_len ? (filename + (filename_len - max_len)) : filename,
            loc.line(),
            max_fcn_len, 
            funname_len > max_fcn_len ? (funname + (funname_len - max_fcn_len)) : funname
        );
        std::print(fmt, std::forward<Args>(args)...);
#endif
    }

#define _lunaris_socket_debug(fmt, ...) Lunaris::Socket::__lunaris_socket_debug(fmt, std::source_location::current() __VA_OPT__(,) __VA_ARGS__)


} // namespace Socket
} // namespace Lunaris