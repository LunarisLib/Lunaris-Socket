#include <Lunaris/platform.h>

#include <Lunaris/exception.h>

namespace Lunaris {
    namespace Socket {
        namespace platform {

            int closesocket(socket_t sock)
            {
                return ::closesocket(sock);
            }

            int ioctlsocket(socket_t sock, int flag, unsigned long mode)
            {
                return ::ioctlsocket(sock, flag, &mode);
            }

            int ioctlsocket(socket_t sock, int flag, int& res)
            {
                u_long tmp;
                const auto fin = ::ioctlsocket(sock, flag, &tmp);
                res = static_cast<int>(tmp);
                return fin;
            }

            int pollsocket(poll_t* pollfd, unsigned long poll_t_size, unsigned timeout_ms)
            {
                return ::WSAPoll(pollfd, poll_t_size, timeout_ms);
            }

            int get_socket_opt(socket_t sock, int level, int opt, int& store)
            {
                socklen_t len = sizeof(store);
                return ::getsockopt(sock, level, opt, (char*)&store, &len);
            }

            int set_socket_opt(socket_t sock, int level, int opt, int val) 
            {
                return ::setsockopt(sock, level, opt, (char*)&val, sizeof(val));
            }

            int set_socket_opt(socket_t sock, int level, int opt, void* val, int len)
            {
                return ::setsockopt(sock, level, opt, (char*)val, len);
            }

            socket_t get_invalid_socket()
            {
                return INVALID_SOCKET;
            }

            bool is_socket_valid(socket_t sock)
            {
                return sock != INVALID_SOCKET;
            }

            bool is_socket_error(socket_t sock)
            {
                return sock == SOCKET_ERROR;
            }

            bool is_socket_timeout(socket_t sock)
            {
                return sock == 0;
            }

            e_errno geterrno()
            {
                switch (WSAGetLastError()) {
                case WSAEWOULDBLOCK:
                    return e_errno::WOULD_BLOCK;
                case WSAENETRESET:
                    return e_errno::NET_RESET;
                case WSAECONNRESET:
                    return e_errno::CONN_RESET;
                default:
                    return e_errno::OTHER;
                }
            }

        } // namespace platform

        namespace detail {

            class WSAInitializer {
            public:
                WSAInitializer() {
                    WSADATA data;
                    if (WSAStartup(MAKEWORD(2, 2), &data) != 0) {
                        throw socket_exception("Cannot initialize winsock!");
                    }
                }

                ~WSAInitializer() {
                    WSACleanup();
                }
            };

            // Global static instance
            static WSAInitializer g_wsa_init;

        } // namespace detail
    } // namespace Socket
} // namespace Lunaris