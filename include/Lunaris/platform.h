#pragma once

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <mstcpip.h>
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")
#else
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
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

#define closeSocket(...) ::closesocket(__VA_ARGS__)
#define ioctlSocket(...) ::ioctlsocket(__VA_ARGS__)
#define pollSocket(...) ::WSAPoll(__VA_ARGS__)
#define theSocketError WSAGetLastError()
#define SocketWOULDBLOCK WSAEWOULDBLOCK
#define SocketNETRESET WSAENETRESET
#define SocketCONNRESET WSAECONNRESET
#define SocketPOLLIN POLLRDNORM
#define SocketProtocolInfo SO_PROTOCOL_INFO
#else
using socket_t = int;
using addr_t = sockaddr;
using addr_info_t = addrinfo;
using addr_storage_t = sockaddr_storage;
using poll_t = pollfd;

constexpr socket_t SOCKET_INVALID_V = -1;
constexpr socket_t SOCKET_ERROR_V = -1;
constexpr socket_t SOCKET_TIMEOUT_V = 0;

#define closeSocket(...) ::close(__VA_ARGS__)
#define ioctlSocket(...) ::ioctl(__VA_ARGS__)
#define pollSocket(...) ::poll(__VA_ARGS__)
#define theSocketError errno
#define SocketWOULDBLOCK EWOULDBLOCK
#define SocketNETRESET ENETRESET
#define SocketCONNRESET ECONNRESET
#define SocketPOLLIN POLLIN
#define SocketProtocolInfo SO_PROTOCOL
#endif
}
}