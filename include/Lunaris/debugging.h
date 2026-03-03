#include <source_location>
#include <print>
#include <format>
#include <string.h>
#include <mutex>

namespace Lunaris {
namespace Socket {


    template <typename... Args>
    inline void __lunaris_socket_debug(
        void* self,
        std::string_view fmt,
        const std::source_location& loc,
        Args&&... args
    ) {
#ifdef LUNARIS_DEBUG
        static std::mutex mtx;

        constexpr size_t max_len = 20;
        constexpr size_t max_fcn_len = 60;

        auto filename = loc.file_name();
        const auto filename_len = strlen(filename);
        bool filename_cut = false;

        auto funname = loc.function_name();
        const auto funname_len = strlen(funname);
        bool funname_cut = false;

        if (filename_len > max_len) {
            const auto diff_to_cut = (filename_len - max_len) + 3;
            filename += diff_to_cut;
            filename_cut = true;
        }

        if (funname_len > max_fcn_len) {
            const auto diff_to_cut = (funname_len - max_fcn_len) + 3;
            funname += diff_to_cut;
            funname_cut = true;
        }

        std::unique_lock<std::mutex> m(mtx);
        std::printf("\n{#%08zX}[%s%*s<>%04u]|@ %s%*s $ ",
            (size_t)self,
            filename_len > max_len ? "..." : "",
            static_cast<int>(max_len - (filename_cut ? 3 : 0)), 
            filename,
            loc.line(),
            funname_len > max_fcn_len ? "..." : "",
            static_cast<int>(max_fcn_len - (funname_cut ? 3 : 0)), 
            funname
        );

        auto tuple = std::forward_as_tuple(std::forward<Args>(args)...);

        std::string msg = std::apply(
            [&](auto&... unpacked) {
                return std::vformat(fmt, std::make_format_args(unpacked...));
            },
            tuple
        );

        std::println("{}", msg);
#endif
    }

#define _lunaris_socket_debug(fmt, ...) Lunaris::Socket::__lunaris_socket_debug(nullptr, fmt, std::source_location::current() __VA_OPT__(,) __VA_ARGS__)
#define _lunaris_socket_debug_c(fmt, ...) Lunaris::Socket::__lunaris_socket_debug((void*)this, fmt, std::source_location::current() __VA_OPT__(,) __VA_ARGS__)


} // namespace Socket
} // namespace Lunaris