#include <Lunaris/platform.h>

#include <sys/ioctl.h>
#include <unistd.h>
#include <errno.h>

namespace Lunaris {
namespace Socket {
    namespace platform {

        int closesocket(socket_t sock)
        {
            return ::close(sock);
        }

        int ioctlsocket(socket_t sock, int flag, unsigned long mode)
        {
            return ::ioctl(sock, flag, mode);
        }

        int ioctlsocket(socket_t sock, int flag, int& res)
        {
            return ::ioctl(sock, flag, &res);
        }

        int pollsocket(poll_t* pollfd, unsigned long poll_t_size, unsigned timeout_ms)
        {
            return ::poll(pollfd, poll_t_size, timeout_ms);
        }

        int get_socket_opt(socket_t sock, int level, int opt, int& store)
        {
            socklen_t len = sizeof(store);
            return ::getsockopt(sock, level, opt, (void*)&store, &len);
        }

        int set_socket_opt(socket_t sock, int level, int opt, int val)
        {
            return ::setsockopt(sock, level, opt, &val, sizeof(val));
        }

        int set_socket_opt(socket_t sock, int level, int opt, void* val, int len)
        {
            return ::setsockopt(sock, level, opt, val, len);
        }

        socket_t get_invalid_socket()
        {
            return -1;
        }
                
        bool is_socket_valid(socket_t sock)
        {
            return sock != -1;
        }

        bool is_socket_error(socket_t sock)
        {
            return sock == -1;
        }

        bool is_socket_timeout(socket_t sock)
        {
            return sock == 0;
        }

        e_errno geterrno()
        {
            switch(errno) {
            case EWOULDBLOCK:
                return e_errno::WOULD_BLOCK;
            case ENETRESET:
                return e_errno::NET_RESET;
            case ECONNRESET:
                return e_errno::CONN_RESET;
            default:
                return e_errno::OTHER;
            }
        }

    } // namespace platform
} // namespace Socket
} // namespace Lunaris