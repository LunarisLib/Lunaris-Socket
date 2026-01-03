#include <Lunaris/socket.h>

#include <stdio.h>
#include <stdexcept>

namespace Lunaris {
namespace Socket {

#ifdef _WIN32
    struct wsa_enable {
        WSAData wsa;
        wsa_enable();
    };

    wsa_enable::wsa_enable() {
        int r = WSAStartup(MAKEWORD(2,2), &wsa); // WSACleanup?
        if (r != 0) throw r;
    }

    static const wsa_enable __g_wsa;
#endif

#pragma region Local tools

    e_family _get_family_from_ip_addr(const char* ip)
    {        
        unsigned char buf[sizeof(struct in6_addr)];

        // Try IPv4
        if (inet_pton(AF_INET, ip, buf) == 1) {
#ifdef LUNARIS_DEBUG_FLAG_TEST
            printf("[create_socket] Deduction: IPV4\n");
#endif
            return e_family::IPV4;
        }

        // Try IPv6
        if (inet_pton(AF_INET6, ip, buf) == 1) {
#ifdef LUNARIS_DEBUG_FLAG_TEST
            printf("[create_socket] Deduction: IPV6\n");
#endif
            return e_family::IPV6;
        }

#ifdef LUNARIS_DEBUG_FLAG_TEST
            printf("[create_socket] Deduction: ANY\n");
#endif
        return e_family::UNSPEC;
    }

    std::unique_ptr<address_storage> _storage_to_class(const addr_storage_t& sg)
    {
        const sockaddr* sa = (struct sockaddr*)(&sg);

        const auto family = sa->sa_family;

        const void* addr_any = (family == AF_INET)
            ? (void*)(&(((struct sockaddr_in*)sa)->sin_addr))
            : (void*)(&(((struct sockaddr_in6*)sa)->sin6_addr));

        const in_port_t port = (family == AF_INET)
            ? (((struct sockaddr_in*)sa)->sin_port)
            : (((struct sockaddr_in6*)sa)->sin6_port);
        
        char final_addr[INET6_ADDRSTRLEN + 1]{};

        if (!inet_ntop(family, addr_any, final_addr, INET6_ADDRSTRLEN * sizeof(char)))
            return {};

        const uint16_t final_port = ntohs(port);

        return std::unique_ptr<address_storage>(new address_storage {
            std::string(final_addr),
            static_cast<e_family>(family),
            final_port 
        });
    }


// Google's
/*
#include <sys/socket.h> // For socket-related functions and structures
#include <iostream>
#include <string>
#include <arpa/inet.h> // For inet_ntop

// Function to get the local address of a socket
void getLocalSocketAddress(int sockfd) {
    sockaddr_storage addr_storage;
    socklen_t addr_len = sizeof(addr_storage);

    if (getsockname(sockfd, (struct sockaddr*)&addr_storage, &addr_len) == -1) {
        perror("getsockname failed");
        return;
    }

    // Now you can work with addr_storage
    // Cast it to the appropriate type based on ss_family
    if (addr_storage.ss_family == AF_INET) {
        struct sockaddr_in* s = (struct sockaddr_in*)&addr_storage;
        char ip_str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(s->sin_addr), ip_str, sizeof(ip_str));
        std::cout << "Local IPv4 Address: " << ip_str << ", Port: " << ntohs(s->sin_port) << std::endl;
    } else if (addr_storage.ss_family == AF_INET6) {
        struct sockaddr_in6* s = (struct sockaddr_in6*)&addr_storage;
        char ip_str[INET6_ADDRSTRLEN];
        inet_ntop(AF_INET6, &(s->sin6_addr), ip_str, sizeof(ip_str));
        std::cout << "Local IPv6 Address: " << ip_str << ", Port: " << ntohs(s->sin6_port) << std::endl;
    } else {
        std::cout << "Unknown address family: " << addr_storage.ss_family << std::endl;
    }
}

// Function to get the remote address of a connected socket
void getRemoteSocketAddress(int sockfd) {
    sockaddr_storage addr_storage;
    socklen_t addr_len = sizeof(addr_storage);

    if (getpeername(sockfd, (struct sockaddr*)&addr_storage, &addr_len) == -1) {
        perror("getpeername failed");
        return;
    }

    // Similar logic as above to process addr_storage
    if (addr_storage.ss_family == AF_INET) {
        struct sockaddr_in* s = (struct sockaddr_in*)&addr_storage;
        char ip_str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(s->sin_addr), ip_str, sizeof(ip_str));
        std::cout << "Remote IPv4 Address: " << ip_str << ", Port: " << ntohs(s->sin_port) << std::endl;
    } else if (addr_storage.ss_family == AF_INET6) {
        struct sockaddr_in6* s = (struct sockaddr_in6*)&addr_storage;
        char ip_str[INET6_ADDRSTRLEN];
        inet_ntop(AF_INET6, &(s->sin6_addr), ip_str, sizeof(ip_str));
        std::cout << "Remote IPv6 Address: " << ip_str << ", Port: " << ntohs(s->sin6_port) << std::endl;
    } else {
        std::cout << "Unknown address family: " << addr_storage.ss_family << std::endl;
    }
}
*/

#pragma endregion



    address_info::address_info(const char* ipaddr, uint16_t port, e_socktype type, bool passive)
    {
        addr_info_t hints;
        memset((void*)&hints, 0, sizeof(hints));

        hints.ai_family = static_cast<int>(_get_family_from_ip_addr(ipaddr));
        hints.ai_socktype = static_cast<int>(type);
        if (passive) hints.ai_flags = AI_NUMERICHOST | AI_PASSIVE;

        _init(ipaddr, port, hints);
    }

    address_info::address_info(const char* ipaddr, uint16_t port, const addr_info_t& hints)
    {
        _init(ipaddr, port, hints);
    }

    address_info::~address_info()
    {
        ::freeaddrinfo(m_info);
    }

    int address_info::get_error() const
    {
        return m_getaddrinfo_err;
    }

    address_info& address_info::filter(e_family fam)
    {
        std::vector<addr_info_t*> tmp;
        for(auto& i : m_filtered) {
            if (i->ai_family == static_cast<int>(fam))
                tmp.push_back(i);
        }
        m_filtered = tmp;
        return *this;
    }

    address_info& address_info::filter(e_socktype typ)
    {
        std::vector<addr_info_t*> tmp;
        for(auto& i : m_filtered) {
            if (i->ai_socktype == static_cast<int>(typ))
                tmp.push_back(i);
        }
        m_filtered = tmp;
        return *this;
    }

    address_info& address_info::filter(e_protocol pro)
    {
        std::vector<addr_info_t*> tmp;
        for(auto& i : m_filtered) {
            if (i->ai_protocol == static_cast<int>(pro))
                tmp.push_back(i);
        }
        m_filtered = tmp;
        return *this;
    }

    address_info& address_info::filter(bool(*fcn)(addr_info_t*))
    {
        if (!fcn) return *this;

        std::vector<addr_info_t*> tmp;
        for(auto& i : m_filtered) {
            if (fcn(i))
                tmp.push_back(i);
        }
        m_filtered = tmp;
        return *this;
    }

    size_t address_info::size() const
    {
        return m_filtered.size();
    }

    address_info::iterator address_info::begin()
    {
        return { m_filtered, 0 };
    }

    address_info::iterator address_info::end()
    {
        return { m_filtered, size() };
    }

    const address_info::iterator address_info::begin() const
    {
        return { m_filtered, 0 };
    }

    const address_info::iterator address_info::end() const
    {
        return { m_filtered, size() };
    }

    void address_info::_init(const char* ipaddr, uint16_t port, const addr_info_t& hints)
    {
        char port_cstr[8]{};
        snprintf(port_cstr, sizeof(port_cstr), "%hu", port);

        if ((m_getaddrinfo_err = getaddrinfo(ipaddr, port_cstr, &hints, &m_info)) == 0)
        {
            for(auto* i = m_info; i != nullptr; i = i->ai_next)
                m_filtered.push_back(i);
        }
    }


    socket address_info::iterator::operator*()
    {
        if (m_curr >= m_ref.size()) return { SOCKET_INVALID_V };
        const addr_info_t* o = m_ref[m_curr];
        return socket(o->ai_family, o->ai_socktype, o->ai_protocol);
    }

    address_info::iterator& address_info::iterator::operator++(int)
    {
        ++m_curr;
        return *this;
    }

    address_info::iterator address_info::iterator::operator++()
    {
        iterator cpy = *this;
        ++m_curr;
        return cpy;
    }

    bool address_info::iterator::operator==(const iterator& oth) const
    {
        return m_curr == oth.m_curr;
    }

    bool address_info::iterator::operator!=(const iterator& oth) const
    {
        return m_curr != oth.m_curr;
    }

    address_info::iterator::iterator(const std::vector<addr_info_t*>& vec, size_t pos)
        : m_ref(vec), m_curr(pos)
    {}


    socket::socket(e_family fam, e_socktype typ, e_protocol pro) 
        : m_sock(::socket(static_cast<int>(fam),static_cast<int>(typ),static_cast<int>(pro)))
    {
        _fill_missing_address_storage();
    }

    socket::socket(int fam, int typ, int pro) 
        : m_sock(::socket(fam, typ, pro))
    {
        _fill_missing_address_storage();
    }

    socket::~socket()
    {
        if (m_sock != SOCKET_INVALID_V)
            closeSocket(m_sock);
    }

    void socket::close()
    {
        if (m_sock != SOCKET_INVALID_V)
            closeSocket(m_sock);

        m_sock = SOCKET_INVALID_V;
    }

    int socket::setopt(int level, int opt, bool en)
    {
        int on = en ? 1 : 0;
        return this->setopt(level, opt, (char*)&on, sizeof(on));
    }

    int socket::setopt(int level, int opt, const void* val, const socklen_t len)
    {
        return ::setsockopt(m_sock, SOL_SOCKET, SO_REUSEADDR, val, len);
    }

    int socket::getopt(int level, int opt, int& res) const
    {
        socklen_t len = sizeof(res);
        return ::getsockopt(m_sock, SOL_SOCKET, SO_PROTOCOL, &res, &len);
    }

    int socket::get_type() const
    {
        int proto = 0;
        return getopt(SOL_SOCKET, SO_TYPE, proto) == 0
            ? proto
            : -1;
    }

    int socket::get_protocol() const
    {
        int proto = 0;
        return getopt(SOL_SOCKET, SocketProtocolInfo, proto) == 0
            ? proto
            : -1;
    }

    const address_storage& socket::get_from() const
    {
        if (!m_sg_from)
            throw std::runtime_error("No valid address storage?");
        
        return *m_sg_from;
    }

    const address_storage& socket::get_to() const
    {
        if (!m_sg_to)
            throw std::runtime_error("No valid address storage?");
        
        return *m_sg_to;
    }
    
    int socket::ioctl(int flag, u_long mode)
    {
        return ioctlSocket(m_sock, flag, mode);
    }

    int socket::listen(int connections)
    {
        if (get_protocol() != static_cast<int>(e_protocol::TCP))
            return -1;

        if (connections < 0) return -1;
        return ::listen(m_sock, connections);
    }

    socket socket::accept()
    {
        if (get_protocol() != static_cast<int>(e_protocol::TCP))
            return { SOCKET_INVALID_V };

        addr_storage_t from{};
        socklen_t from_len = sizeof(addr_storage_t);

        return { ::accept(m_sock, (addr_t*)&from, &from_len) };
    }

    bool socket::valid() const
    {
        return m_sock != SOCKET_INVALID_V && m_sock != SOCKET_ERROR_V;
    }

    socket::operator bool() const
    {
        return m_sock != SOCKET_INVALID_V && m_sock != SOCKET_ERROR_V;
    }


    socket::socket(socket_t sock)
        : m_sock(sock)
    {
        _fill_missing_address_storage();
    }
    
    socket::socket(socket_t sock, std::unique_ptr<address_storage>&& addr)
        : m_sock(sock), m_sg_from(std::move(addr))
    {

        _fill_missing_address_storage();
    }

    void socket::_fill_missing_address_storage()
    {
        addr_storage_t addr_storage;
#ifdef _WIN32
        int addr_len = sizeof(addr_storage);
#else
        socklen_t addr_len = sizeof(addr_storage);
#endif

        memset(&addr_storage, 0, sizeof(addr_storage));

        if (!m_sg_to && getpeername(m_sock, (addr_t*)&addr_storage, &addr_len) == 0) {
            m_sg_to = _storage_to_class(addr_storage);
        }

        memset(&addr_storage, 0, sizeof(addr_storage));

        if (!m_sg_from && getsockname(m_sock, (struct sockaddr*)&addr_storage, &addr_len) == 0) {
            m_sg_from = _storage_to_class(addr_storage);
        }
    }




}
}