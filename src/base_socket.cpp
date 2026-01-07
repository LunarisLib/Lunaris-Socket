#include <arpa/inet.h>
#include <string.h>

#include <Lunaris/socket.h>
#include <Lunaris/exception.h>

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
        socklen_t len = sizeof(res);
        return ::getsockopt(m_sock->sock, SOL_SOCKET, SO_PROTOCOL, &res, &len);
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

        const in_port_t port = (family == AF_INET)
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

    BaseSocket::sock_info::sock_info(socket_t s, e_socktype type, addr_storage_t* addr, socklen_t len)
        : sock(s), type(type), storage({}), storage_len(static_cast<socklen_t>(len)), owns_socket(true)
    {
        ::memset(&storage, 0, sizeof(storage));
        ::memcpy(&storage, addr, storage_len);
    }

    BaseSocket::sock_info::~sock_info()
    {
        if (owns_socket && platform::is_socket_valid(sock))
            platform::closesocket(sock);
        sock = platform::get_invalid_socket();
    }

    std::unique_ptr<BaseSocket::sock_info> BaseSocket::sock_info::make_ref() const
    {
        auto cpy = std::make_unique<sock_info>(sock, type, (addr_storage_t*)&storage, storage_len);
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

            auto dev = std::make_unique<sock_info>(s, type, (addr_storage_t*)AI->ai_addr, AI->ai_addrlen);

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

        void* any_addr = using_v6 ? (void*)new sockaddr_in6() : (void*)new sockaddr_in();
        const size_t any_addr_len = using_v6 ? sizeof(sockaddr_in6) : sizeof(sockaddr_in);

        if (using_v6) {
            sockaddr_in6& addr = *(sockaddr_in6*)any_addr;
            addr.sin6_family = AF_INET6;
            addr.sin6_port   = htons(port);
            addr.sin6_addr   = in6addr_any;
        }
        else {
            sockaddr_in& addr = *(sockaddr_in*)any_addr;
            addr.sin_family      = AF_INET;
            addr.sin_port        = htons(port);
            addr.sin_addr.s_addr = INADDR_ANY;
        }

        auto dev = std::make_unique<sock_info>(s, type, (addr_storage_t*)any_addr, any_addr_len);

        if (family == e_family::UNSPEC) {
            int off = 0;
            if (::setsockopt(dev->sock, IPPROTO_IPV6, IPV6_V6ONLY, &off, sizeof(off)) != 0) {
                throw socket_exception("Could not instantiate host socket - failed to enable dual stack"); // implicit close socket on dev
            }
        }

        {
            int reuse = 1;
            if (::setsockopt(dev->sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) != 0) {
                throw socket_exception("Could not instantiate host socket - failed to enable reuse addr"); // implicit close socket on dev                    
            }
        }

        if (::bind(dev->sock, (addr_t*)any_addr, any_addr_len) != 0) {
            throw socket_exception("Could not instantiate host socket - failed to bind to desired config"); // implicit close socket on dev
        }

        if (dev->type == e_socktype::STREAM) {
            if (::listen(dev->sock, 5) != 0) {
                throw socket_exception("Could not instantiate host socket - failed to enable listen on STREAM socket"); // implicit close socket on dev
            }
        }

        m_sock = std::move(dev);
    }

    /*ClientSocket HostSocket::accept() const
    {
        addr_storage_t storage = {};
        socklen_t storage_len = sizeof(addr_storage_t);

        if (m_sock->type == e_socktype::STREAM) {
            auto s = ::accept(m_sock->sock, (addr_t*)&storage, &storage_len);

            if (!platform::is_socket_valid(s))
                throw socket_exception("Could not instantiate accepted socket - failed to create socket");

            auto info = std::make_unique<sock_info>(s, m_sock->type, storage, storage_len);

            return ClientSocket{ std::move(info) };
        }
        else {
            auto info = m_sock->make_ref();
            char dummy;

            if (::recvfrom( /// HMM ACTUALLY THIS WON'T WORK AS WE EXPECT IT TO WORK... It does not keep the reference, so... recv() on client won't do
                info->sock,
                &dummy,
                1,
                MSG_PEEK,
                (addr_t*)&info->storage,
                &info->storage_len
            ) < 1) {
                throw socket_exception("Could not instantiate accepted socket - recvfrom did not work correctly");
            }

            return ClientSocket{ std::move(info) };
        }
    }*/

#pragma endregion

} // namespace Base
} // namespace Socket
} // namespace Lunaris