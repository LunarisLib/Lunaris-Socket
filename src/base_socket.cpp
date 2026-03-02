#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <mstcpip.h>
#else
#include <arpa/inet.h>
#endif

#include <string.h>

#include <Lunaris/socket.h>
#include <Lunaris/exception.h>
#include <Lunaris/debugging.h>

namespace Lunaris {
namespace Socket {
namespace Base {

#pragma region Local tools hidden

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

#pragma region Base Socket - shared functions

    bool BaseSocket::valid() const
    {
        return m_sock && platform::is_socket_valid(m_sock->sock);
    }
    
    BaseSocket::operator bool() const
    {
        return m_sock && platform::is_socket_valid(m_sock->sock);
    }

    int BaseSocket::getopt(int level, int opt, int& res) const
    {
        if (!m_sock) return -1;
        return platform::get_socket_opt(m_sock->sock, level, opt, res);
    }

    e_socktype BaseSocket::get_type() const
    {
        int proto = 0;
        return this->getopt(SOL_SOCKET, SO_TYPE, proto) == 0
            ? static_cast<e_socktype>(proto)
            : e_socktype::DGRAM;
    }

    e_family BaseSocket::get_family() const
    {
        if (!m_sock) return e_family::UNSPEC;
        const sockaddr* sa = (struct sockaddr*)(&m_sock->storage);
        return static_cast<e_family>(sa->sa_family);

    }

    int BaseSocket::ioctl(int flag, u_long mode)
    {
        return m_sock ? platform::ioctlsocket(m_sock->sock, flag, mode) : -1;
    }

    address_storage BaseSocket::get_config() const
    {
        if (!m_sock) return {};

        const sockaddr* sa = (struct sockaddr*)(&m_sock->storage);

        const auto family = sa->sa_family;

        const void* addr_any = (family == AF_INET)
            ? (void*)(&(((struct sockaddr_in*)sa)->sin_addr))
            : (void*)(&(((struct sockaddr_in6*)sa)->sin6_addr));

        const uint16_t port = (family == AF_INET)
            ? (((struct sockaddr_in*)sa)->sin_port)
            : (((struct sockaddr_in6*)sa)->sin6_port);
        
        char final_addr[INET6_ADDRSTRLEN + 1]{};

        if (!::inet_ntop(family, addr_any, final_addr, INET6_ADDRSTRLEN * sizeof(char)))
            return {};

        const uint16_t final_port = ::ntohs(port);

        return {
            std::string(final_addr),
            static_cast<e_family>(family),
            final_port 
        };
    }

    void BaseSocket::close()
    {
        m_sock.reset();
    }

    BaseSocket::sock_info::sock_info(socket_t s, e_socktype type, addr_storage_t* addr, socklen_t len)
        : sock(s), type(type), storage({}), storage_len(static_cast<socklen_t>(len)), owns_socket(true)
    {
        ::memset(&storage, 0, sizeof(storage));
        ::memcpy(&storage, addr, storage_len);
#ifdef _WIN32
        GUID guid = WSAID_WSARECVMSG;
        LPFN_WSARECVMSG fn = nullptr;
        DWORD bytes = 0;

        if (WSAIoctl(s,
            SIO_GET_EXTENSION_FUNCTION_POINTER,
            &guid,
            sizeof(guid),
            &fn,
            sizeof(fn),
            &bytes,
            nullptr,
            nullptr) != SOCKET_ERROR)
        {
            this->wsarecvmsg_fn = fn;
        }
        else {
            this->wsarecvmsg_fn = nullptr;
        }
#endif
        _lunaris_socket_debug_c("sock_info: new sock_info, sock={:08X} type={}", (uint64_t)s, (uint64_t)type);
    }

#ifdef _WIN32
    BaseSocket::sock_info::sock_info(socket_t s, e_socktype type, addr_storage_t* addr, socklen_t len, LPFN_WSARECVMSG fn)
        : sock(s), type(type), storage({}), storage_len(static_cast<socklen_t>(len)), owns_socket(true), wsarecvmsg_fn(fn)
    {
        ::memset(&storage, 0, sizeof(storage));
        ::memcpy(&storage, addr, storage_len);
        _lunaris_socket_debug_c("sock_info: new sock_info, sock={:08X} type={}", (uint64_t)s, (uint64_t)type);
    }
#endif

    BaseSocket::sock_info::~sock_info()
    {
        if (owns_socket && platform::is_socket_valid(sock)){
            _lunaris_socket_debug_c("sock_info: deleted sock_info, sock={:08X}", (uint64_t)sock);
            platform::closesocket(sock);
        }
        _lunaris_socket_debug_c("sock_info: ref removed of sock_info, sock={:08X}", (uint64_t)sock);
        sock = platform::get_invalid_socket();
    }

    std::unique_ptr<BaseSocket::sock_info> BaseSocket::sock_info::make_ref() const
    {
        _lunaris_socket_debug_c("sock_info: referencing sock_info, sock={:08X}", (uint64_t)sock);
#ifdef _WIN32
        auto cpy = std::make_unique<sock_info>(sock, type, (addr_storage_t*)&storage, storage_len, wsarecvmsg_fn);
#else
        auto cpy = std::make_unique<sock_info>(sock, type, (addr_storage_t*)&storage, storage_len);
#endif
        cpy->owns_socket = false;
        return cpy;
    }

    bool BaseSocket::sock_info::operator==(const sock_info& o) const
    {
        return ::memcmp(this, &o, sizeof(*this)) == 0;
    }

    bool BaseSocket::sock_info::operator!=(const sock_info& o) const
    {
        return ::memcmp(this, &o, sizeof(*this)) != 0;
    }

#pragma endregion

#pragma region Client Socket specific

    ClientSocket::ClientSocket(const char* address, uint16_t port, e_socktype type)
    {
        addr_info_t* info;
        addr_info_t hints;
        memset((void*)&hints, 0, sizeof(hints));

        hints.ai_family = static_cast<int>(_get_family_from_ip_addr(address));
        hints.ai_socktype = static_cast<int>(type);

        char port_cstr[8]{};
        snprintf(port_cstr, sizeof(port_cstr), "%hu", port);

        if (getaddrinfo(address, port_cstr, &hints, &info) != 0)
            throw socket_exception("Could not instantiate client socket - could not resolve address, port or other config");

        for (auto AI = info; AI != nullptr; AI = AI->ai_next) {
            socket_t s = ::socket(AI->ai_family, AI->ai_socktype, AI->ai_protocol);
            if (!platform::is_socket_valid(s)) continue;

            auto dev = std::make_unique<sock_info>(s, type, (addr_storage_t*)AI->ai_addr, static_cast<socklen_t>(AI->ai_addrlen));

            if (type == e_socktype::STREAM) {
                if (platform::is_socket_error(::connect(dev->sock, AI->ai_addr, AI->ai_addrlen))) {
                    continue; // dev implicitly closes socket here
                }
            }
            
            ::freeaddrinfo(info);
            m_sock = std::move(dev);
            return;
        }

        ::freeaddrinfo(info);
        throw socket_exception("Could not instantiate client socket - failed to find suitable config");
    }


    ClientSocket::ClientSocket(std::unique_ptr<BaseSocket::sock_info>&& pre_cfg)
    {
        m_sock = std::move(pre_cfg);
    }

#pragma endregion

#pragma region Host Socket specific

    HostSocket::HostSocket(uint16_t port, e_family family, e_socktype type)
    {
        const bool using_v6 = family != e_family::IPV4;

        socket_t s = ::socket(
            using_v6 ? AF_INET6 : AF_INET, // both IPV6 or dual use IPV6 as base
            static_cast<int>(type),
            0
        );

        if (!platform::is_socket_valid(s))
            throw socket_exception("Could not instantiate host socket - failed to create socket");

        //void* any_addr = using_v6 ? (void*)new sockaddr_in6() : (void*)new sockaddr_in();
        addr_storage_t any_addr{};
        const size_t any_addr_len = using_v6 ? sizeof(sockaddr_in6) : sizeof(sockaddr_in);

        if (using_v6) {
            auto& addr = reinterpret_cast<sockaddr_in6&>(any_addr);
            addr.sin6_family = AF_INET6;
            addr.sin6_port   = htons(port);
            addr.sin6_addr   = in6addr_any;
        }
        else {
            auto& addr = reinterpret_cast<sockaddr_in&>(any_addr);
            addr.sin_family      = AF_INET;
            addr.sin_port        = htons(port);
            addr.sin_addr.s_addr = INADDR_ANY;
        }

        auto dev = std::make_unique<sock_info>(s, type, (addr_storage_t*)&any_addr, static_cast<socklen_t>(any_addr_len));

        if (family == e_family::UNSPEC) {
            if (platform::set_socket_opt(dev->sock, IPPROTO_IPV6, IPV6_V6ONLY, 0) != 0) {
                throw socket_exception("Could not instantiate host socket - failed to enable dual stack"); // implicit close socket on dev
            }
        }

        if (platform::set_socket_opt(dev->sock, SOL_SOCKET, SO_REUSEADDR_AUTO, 1) != 0) {
            throw socket_exception("Could not instantiate host socket - failed to enable reuse addr"); // implicit close socket on dev                    
        }

        if (::bind(dev->sock, (addr_t*)&any_addr, any_addr_len) != 0) {
            throw socket_exception("Could not instantiate host socket - failed to bind to desired config"); // implicit close socket on dev
        }

        if (dev->type == e_socktype::STREAM) {
            if (::listen(dev->sock, 5) != 0) {
                throw socket_exception("Could not instantiate host socket - failed to enable listen on STREAM socket"); // implicit close socket on dev
            }
        }

        m_sock = std::move(dev);
    }

#pragma endregion

} // namespace Base
} // namespace Socket
} // namespace Lunaris