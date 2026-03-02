#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <string.h>

#include <Lunaris/tcp_socket.h>
#include <Lunaris/exception.h>

namespace Lunaris {
namespace Socket {

#pragma region Local tools hidden

    extern e_family _get_family_from_ip_addr(const char* ip);

#pragma endregion

#pragma region TCP Area

    TCP_Client::TCP_Client(const char* address, const uint16_t port)
        : ClientSocket(address, port, e_socktype::STREAM)
    {}

    ptrdiff_t TCP_Client::send(const char* data, const size_t len) const
    {
        return m_sock ? ::send(m_sock->sock, data, len, 0) : -2;
    }

    ptrdiff_t TCP_Client::recv(char* data, const size_t len) const
    {
        return m_sock ? ::recv(m_sock->sock, data, len, 0) : -2;
    }

    ptrdiff_t TCP_Client::recv_autowait(char* data, const size_t len) const
    {
        if (!m_sock) return -2;

        ptrdiff_t accum = 0;
        while(accum < len) {
            ptrdiff_t got = this->recv(data + accum, len - accum);
            if (got <= 0) return got;
            accum += got;
        }

        return accum;
    }

    size_t TCP_Client::recv_size() const
    {
        int res = 0;
        if (platform::ioctlsocket(m_sock->sock, FIONREAD, res) < 0) return 0;
        return static_cast<size_t>(res);
    }

    TCP_Host::TCP_Host(uint16_t port, e_family family)
        : HostSocket(port, family, e_socktype::STREAM)
    {}

    TCP_Client TCP_Host::accept() const
    {
        addr_storage_t storage = {};
        socklen_t storage_len = sizeof(addr_storage_t);

        auto s = ::accept(m_sock->sock, (addr_t*)&storage, &storage_len);

        if (!platform::is_socket_valid(s))
            throw socket_exception("Could not instantiate accepted socket - failed to create socket");

        auto info = std::make_unique<sock_info>(s, m_sock->type, &storage, storage_len);

        return TCP_Client{ std::move(info) };
    }

#pragma endregion

} // namespace Socket
} // namespace Lunaris