#pragma once

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#include <ws2def.h>
#else
#include <sys/socket.h>
#include <sys/poll.h>
#include <netdb.h>
#endif

namespace Lunaris {
namespace Socket {

#ifdef _WIN32
    using socket_t = SOCKET;
    using addr_t = SOCKADDR;
    using addr_info_t = ADDRINFO;
    using addr_storage_t = SOCKADDR_STORAGE;
    using poll_t = WSAPOLLFD;
    using name_len = socklen_t;
    using message = WSAMSG;
    using io_buf = WSABUF;
#define const_message_data WSA_CMSG_DATA

    constexpr auto IPV6_RECV_PKT_INFO = IPV6_PKTINFO;
    constexpr auto SO_REUSEADDR_AUTO = SO_REUSEADDR;
#else
    using socket_t = int;
    using addr_t = sockaddr;
    using addr_info_t = addrinfo;
    using addr_storage_t = sockaddr_storage;
    using poll_t = pollfd;
    using name_len = socklen_t;
    using message = msghdr;
    using io_buf = iovec;
#define const_message_data CMSG_DATA

    constexpr auto IPV6_RECV_PKT_INFO = IPV6_RECVPKTINFO;
    constexpr auto SO_REUSEADDR_AUTO = SO_REUSEADDR;
#endif

    enum class e_errno : int {
        OTHER,
        WOULD_BLOCK,
        NET_RESET,
        CONN_RESET
    };

    namespace platform {
        int closesocket(socket_t sock);
        int ioctlsocket(socket_t sock, int flag, unsigned long mode);
        int ioctlsocket(socket_t sock, int flag, int& res);
        int pollsocket(poll_t* pollfd, unsigned long poll_t_size, unsigned timeout_ms);
        int get_socket_opt(socket_t, int, int, int&); // getsockopt
        int set_socket_opt(socket_t, int, int, int); // setsockopt
        int set_socket_opt(socket_t, int, int, void*, int); // setsockopt

        socket_t get_invalid_socket(void);

        bool is_socket_valid(socket_t sock);
        bool is_socket_error(socket_t sock);
        bool is_socket_timeout(socket_t sock);

        e_errno geterrno();
    } // namespace platform
} // namespace Socket
} // namespace Lunaris