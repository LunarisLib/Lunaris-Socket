#pragma once

#include <Lunaris/base_socket.h>

namespace Lunaris {
namespace Socket {

    class TCP_Host;

    class TCP_Client : protected Base::ClientSocket {
    public:
        TCP_Client(const char* address, uint16_t port);
        ~TCP_Client() override = default;
        
        ssize_t send(const char*, const size_t) const;
        ssize_t recv(char*, const size_t) const;

        // if package fragment, it will continue waiting for what comes next to complete the full size requested
        ssize_t recv_autowait(char*, const size_t) const;

        // in buffer to recv right now
        size_t recv_size() const;

        using Base::BaseSocket::valid;
        using Base::BaseSocket::operator bool;
        using Base::BaseSocket::get_type;
        using Base::BaseSocket::get_family;
        using Base::BaseSocket::get_config;
        using Base::BaseSocket::close;
    private:
        using Base::ClientSocket::ClientSocket;

        friend class TCP_Host;
    };

    class TCP_Host : protected Base::HostSocket {
    public:
        TCP_Host(uint16_t port, e_family family = e_family::UNSPEC);
        ~TCP_Host() override = default;

        TCP_Client accept() const;

        using Base::BaseSocket::valid;
        using Base::BaseSocket::operator bool;
        using Base::BaseSocket::get_type;
        using Base::BaseSocket::get_family;
        using Base::BaseSocket::get_config;
        using Base::BaseSocket::close;
    };

} // namespace Socket
} // namespace Lunaris