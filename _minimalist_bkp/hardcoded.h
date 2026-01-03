#include <Lunaris/platform.h>

#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <vector>
#include <string>
#include <memory>
#include <algorithm>

using namespace Lunaris::Socket;

enum class e_family {
    IPV4 = PF_INET,
    IPV6 = PF_INET6,
    UNSPEC = PF_UNSPEC
};

enum class e_socktype {
    STREAM = SOCK_STREAM,
    DGRAM = SOCK_DGRAM,
};

enum class e_protocol {
    UNSPEC = 0,
    TCP = IPPROTO_TCP,
    UDP = IPPROTO_UDP
};


struct socket_device {
    socket_t sock = platform::get_invalid_socket();
    e_socktype type = e_socktype::DGRAM;
    addr_storage_t storage = {};
    socklen_t storage_len = 0;

    socket_device() = default;
    socket_device(const socket_device& o) {
        memcpy(this, &o, sizeof(socket_device));
    }
    void operator=(const socket_device& o) {
        memcpy(this, &o, sizeof(socket_device));
    }
};

#pragma region Local tools for this file

e_family _get_family_from_ip_addr(const char* ip)
{
    if (!ip) {
        return e_family::UNSPEC;
    }

    unsigned char buf[sizeof(struct in6_addr)];

    if (inet_pton(AF_INET, ip, buf) == 1) {
        return e_family::IPV4;
    }

    if (inet_pton(AF_INET6, ip, buf) == 1) {
        return e_family::IPV6;
    }

    return e_family::UNSPEC;
}

#pragma endregion


socket_device create_host_socket(uint16_t port, e_socktype type) {

    socket_t s = ::socket(AF_INET6, static_cast<int>(type), 0);
    if (!platform::is_socket_valid(s))
        return {};

    int off = 0;
    if (::setsockopt(s, IPPROTO_IPV6, IPV6_V6ONLY, &off, sizeof(off)) != 0) {
        platform::closesocket(s);
        return {};
    }

    int reuse = 1;
    ::setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    sockaddr_in6 addr{};
    addr.sin6_family = AF_INET6;
    addr.sin6_port   = htons(port);
    addr.sin6_addr   = in6addr_any;

    if (::bind(s, (addr_t*)&addr, sizeof(addr)) != 0) {
        platform::closesocket(s);
        return {};
    }

    if (type == e_socktype::STREAM) {
        if (::listen(s, 5) != 0) {
            platform::closesocket(s);
            return {};
        }
    }

    socket_device dev{};
    dev.sock = s;
    dev.type = type;
    dev.storage_len = sizeof(addr);
    memcpy(&dev.storage, &addr, sizeof(addr));

    return dev;
}

socket_device accept_host_auto(socket_device& s)
{
    socket_device dev;
    dev.type = s.type;
    dev.storage_len = sizeof(addr_storage_t);

    if (dev.type == e_socktype::STREAM) {
        dev.sock = ::accept(s.sock, (addr_t*)&dev.storage, &dev.storage_len);
    }
    else {
        dev.sock = s.sock;
        char dummy;

        if (::recvfrom(
            dev.sock,
            &dummy,
            1,
            MSG_PEEK,
            (addr_t*)&dev.storage,
            &dev.storage_len
        ) < 1) {
            return {};
        }
    }

    return dev;
}

socket_device create_client_socket(const char* path, uint16_t port, e_socktype type) {    
    addr_info_t* info;
    addr_info_t hints;
    memset((void*)&hints, 0, sizeof(hints));

    hints.ai_family = static_cast<int>(_get_family_from_ip_addr(path));
    hints.ai_socktype = static_cast<int>(type);

    char port_cstr[8]{};
    snprintf(port_cstr, sizeof(port_cstr), "%hu", port);

    if (getaddrinfo(path, port_cstr, &hints, &info) != 0)
        return {};

    for (auto AI = info; AI != nullptr; AI = AI->ai_next) {
        socket_t s = ::socket(AI->ai_family, AI->ai_socktype, AI->ai_protocol);
        if (!platform::is_socket_valid(s)) continue;

        socket_device dev{};
        dev.sock = s;

        // Copy address safely
        memcpy(&dev.storage, AI->ai_addr, AI->ai_addrlen);
        dev.storage_len = static_cast<socklen_t>(AI->ai_addrlen);
        dev.type = type;

        if (type == e_socktype::STREAM) {
            if (platform::is_socket_error(::connect(s, AI->ai_addr, AI->ai_addrlen))) {
                platform::closesocket(s);
                continue;
            }
        }
        
        freeaddrinfo(info);
        return dev;
    }

    freeaddrinfo(info);
    return {};
}