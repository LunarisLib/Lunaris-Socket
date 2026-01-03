#pragma once

#include <sys/socket.h>
#include <sys/poll.h>
#include <netdb.h>

namespace Lunaris {
namespace Socket {

    // Linux mapping
    using socket_t = int;
    using addr_t = sockaddr;
    using addr_info_t = addrinfo;
    using addr_storage_t = sockaddr_storage;
    using poll_t = pollfd;
    using name_len = socklen_t;

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

        socket_t get_invalid_socket();
        int get_socket_protocol_info_num();

        bool is_socket_valid(socket_t sock);
        bool is_socket_error(socket_t sock);
        bool is_socket_timeout(socket_t sock);

        e_errno geterrno();
    } // namespace platform
} // namespace Socket
} // namespace Lunaris