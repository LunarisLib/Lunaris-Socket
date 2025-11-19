#include <Lunaris/socket.h>
#include <Lunaris/types.h>

#include <stdexcept>
#include <cstring>
#include <vector>
#include <algorithm>
#include <memory>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdexcept>
#pragma comment (lib, "Ws2_32.lib")
#else
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <unistd.h>
#endif

#define LUNARIS_DEBUG_FLAG_TEST

namespace Lunaris {
    namespace Socket {

        void _ensure_winsock() {
#ifdef _WIN32
            static bool initialized = false;
            if (initialized) return;
            
            WSAData wsa;
            int r = WSAStartup(MAKEWORD(2,2), &wsa); // WSACleanup?
            if (r != 0) {
                throw std::runtime_error("WSAStartup failed");
            }
            else initialized = true;                
#endif
        }

        int _get_family_from_ip_addr(const std::string& ip)
        {
            _ensure_winsock();
            
            unsigned char buf[sizeof(struct in6_addr)];

            // Try IPv4
            if (inet_pton(AF_INET, ip.c_str(), buf) == 1) {
#ifdef LUNARIS_DEBUG_FLAG_TEST
                printf("[create_socket] Deduction: IPV4\n");
#endif
                return PF_INET;
            }

            // Try IPv6
            if (inet_pton(AF_INET6, ip.c_str(), buf) == 1) {
#ifdef LUNARIS_DEBUG_FLAG_TEST
                printf("[create_socket] Deduction: IPV6\n");
#endif
                return PF_INET6;
            }

#ifdef LUNARIS_DEBUG_FLAG_TEST
                printf("[create_socket] Deduction: ANY\n");
#endif
            return PF_UNSPEC;
        }

        std::vector<socket_t> _make_socks(std::string addr, uint16_t port, int protocol, bool host = false)
        {
            _ensure_winsock();

            std::vector<socket_t> m_socks;

            const char* final_addr = host
                ? nullptr
                : (addr.empty() ? "localhost" : addr.c_str());

#ifdef LUNARIS_DEBUG_FLAG_TEST
            printf("[create_socket] Begun: addr:port='%s:%hu' protocol=%d\n", final_addr ? final_addr : "null", port, protocol);
#endif
            char port_cstr[8]{};
            addr_info_t hints;
            addr_info_t* addr_info = nullptr;
            const size_t socks_lim = host ? static_cast<size_t>(FD_SETSIZE) : 1;

            snprintf(port_cstr, 8, "%hu", port);
            memset((void*)&hints, 0, sizeof(hints));

            hints.ai_family = _get_family_from_ip_addr(addr);
            hints.ai_socktype = protocol;
            if (host) hints.ai_flags = AI_NUMERICHOST | AI_PASSIVE;

#ifdef LUNARIS_DEBUG_FLAG_TEST
            printf("[create_socket] Automatic deduction of family: %i\n", hints.ai_family);
            printf("[create_socket] Getting info...\n");
#endif

            if (int err = getaddrinfo(final_addr, port_cstr, &hints, &addr_info); err != 0) {
#ifdef LUNARIS_DEBUG_FLAG_TEST
                printf("[create_socket] Error getting addr info: %i\n", err);
#endif
                throw std::runtime_error("Error getting addr info!");
            }

#ifdef LUNARIS_DEBUG_FLAG_TEST
            printf("[create_socket] Iterating through options...\n");
#endif

            for (addr_info_t* opt = addr_info; opt != nullptr && m_socks.size() < socks_lim; opt = opt->ai_next)
            {
                if (opt->ai_family != PF_INET && opt->ai_family != PF_INET6)
                    continue;

                socket_t sock = ::socket(opt->ai_family, opt->ai_socktype, opt->ai_protocol);

                if (sock == SOCKET_INVALID_V) 
                    continue;

                if (host) {
                    int on = 1;
                    ::setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char*)&on, sizeof(on));

                    if (::bind(sock, opt->ai_addr, (int)opt->ai_addrlen) == SOCKET_ERROR_V) {
#ifdef _WIN32
                        ::closesocket(sock);
#else
                        ::close(sock);
#endif
                        continue;
                    }

                    // Specific for TCP
                    if (protocol == SOCK_STREAM) {
                        if (::listen(sock, 5) == SOCKET_ERROR_V) {
#ifdef _WIN32
                            ::closesocket(sock);
#else
                            ::close(sock);
#endif
                            continue;
                        }
                    }

			        m_socks.push_back(sock);
                }
                else {
                    if (::connect(sock, opt->ai_addr, (int)opt->ai_addrlen) == SOCKET_ERROR_V) {
#ifdef _WIN32
                        ::closesocket(sock);
#else
                        ::close(sock);
#endif
                        continue;
                    }

                    if (sock != SOCKET_INVALID_V)
                        m_socks.push_back(sock);
                }
            }

            freeaddrinfo(addr_info);

#ifdef LUNARIS_DEBUG_FLAG_TEST
            if (m_socks.size() == 0) {
                printf("[create_socket] Got invalid socket(s). Could not connect to any.\n");
            }
            else {
                printf("[create_socket] Got valid socket(s)!\n");
            }
#endif
            return m_socks;
        }

        void _close_sock(socket_t& m_sock)
        {
            if (m_sock == SOCKET_INVALID_V) return;
#ifdef _WIN32
            ::closesocket(m_sock);
#else
            ::close(m_sock);
#endif
            m_sock = SOCKET_INVALID_V;
        }

        socket_t _select_socks(const std::vector<socket_t>& m_socks, long timeout_ms)
        {
            if (m_socks.size() == 0) return SOCKET_INVALID_V;

            const unsigned long nfds = static_cast<unsigned long>(m_socks.size());

            auto pul = std::unique_ptr<poll_t[]>(new poll_t[nfds]);
            memset(pul.get(), 0, sizeof(poll_t) * nfds);

            for (size_t p = 0; p < nfds; p++) {
                pul[p].events = POLLRDNORM;
                pul[p].fd = m_socks[p];
            }

#ifdef _WIN32
            int res = ::WSAPoll(pul.get(), nfds, timeout_ms > 0 ? timeout_ms : -1);
#else
            int res = ::poll(pul.get(), nfds, timeout_ms > 0 ? timeout_ms : -1);
#endif

            if (res < 0) {
                return SOCKET_INVALID_V;
            }

            for (size_t pp = 0; pp < nfds; pp++) {
                auto& it = pul[pp];
                if (it.revents != POLLRDNORM) continue; // not "read data available"
                if (std::find(m_socks.begin(), m_socks.end(), it.fd) != m_socks.end()) return it.fd; // this is the one.
            }

            return SOCKET_INVALID_V;
        }

        size_t _get_recv_size(socket_t sock)
        {
            u_long arg{};
#ifdef _WIN32
            ::ioctlsocket(sock, FIONREAD, &arg);
#else
            ::ioctl(sock, FIONREAD, &arg);
#endif
            return static_cast<size_t>(arg);
        }

    }
}