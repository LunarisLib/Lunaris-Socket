#pragma once

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#pragma comment (lib, "Ws2_32.lib")
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

        constexpr socket_t SOCKET_INVALID_V = INVALID_SOCKET;
        constexpr socket_t SOCKET_ERROR_V = SOCKET_ERROR;
        constexpr socket_t SOCKET_TIMEOUT_V = 0;
#else
        using socket_t = int;
        using addr_t = sockaddr;
        using addr_info_t = addrinfo;
        using addr_storage_t = sockaddr_storage;
        using poll_t = pollfd;

        constexpr socket_t SOCKET_INVALID_V = -1;
        constexpr socket_t SOCKET_ERROR_V = -1;
        constexpr socket_t SOCKET_TIMEOUT_V = 0;
#endif


    }
}