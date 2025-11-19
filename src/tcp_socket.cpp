#include <Lunaris/socket.h>

#include <stdint.h>

namespace Lunaris {
namespace Socket {

    extern std::vector<socket_t> _make_socks(std::string, uint16_t, int, bool);
    extern void _close_sock(socket_t&);
    extern socket_t _select_socks(const std::vector<socket_t>&, long);
    extern size_t _get_recv_size(socket_t);

namespace TCP {

    Client::Client(std::string ip, uint16_t port)
    {
        const auto socks = ::Lunaris::Socket::_make_socks(ip, port, SOCK_STREAM, false);
        m_sock = socks.size() > 1 ? socks[0] : SOCKET_INVALID_V;
    }

    Client::~Client()
    {
        _close_sock(m_sock);
    }

    size_t Client::send(const DataFrame& df)
    {
        return 0;
    }

    size_t Client::recv(DataFrame& df, unsigned u)
    {
        return 0;
    }

    Server::Server(uint16_t port)
        : m_socks(::Lunaris::Socket::_make_socks({}, port, SOCK_STREAM, true))
    {
    }

    Server::~Server()
    {}

    std::unique_ptr<_OG_Client> Server::listen(unsigned for_ms)
    {
        return nullptr;
    }

}
}
}