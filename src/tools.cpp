#include <Lunaris/tools.h>

#include <regex>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdexcept>
#pragma comment (lib, "Ws2_32.lib")
#else
#include <arpa/inet.h>
#include <sys/socket.h>
#endif

namespace Lunaris {
    namespace Socket {
        namespace Tools {

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

            int get_family_from_ip_addr(const std::string& ip)
            {
                _ensure_winsock();
                
                unsigned char buf[sizeof(struct in6_addr)];

                // Try IPv4
                if (inet_pton(AF_INET, ip.c_str(), buf) == 1)
                    return PF_INET;

                // Try IPv6
                if (inet_pton(AF_INET6, ip.c_str(), buf) == 1)
                    return PF_INET6;

                return PF_UNSPEC;
            }

        }
    }
}