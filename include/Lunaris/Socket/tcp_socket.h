#pragma once

#include <Lunaris/Socket/base_socket.h>

namespace Lunaris {
namespace Socket {

    class TCPHost;

    /**
     * @brief TCP Client class for TCP connections to servers
     */
    class TCPClient : protected Base::ClientSocket {
    public:
        /**
         * @brief Construct a new TCP client object
         * 
         * @param address The address to connect to
         * @param port The port used for connection
         */
        TCPClient(const char* address, uint16_t port);
        ~TCPClient() override = default;
        
        /**
         * @brief Send a block of data through the socket
         * 
         * @param data The data to send
         * @param len The size of the data
         * @return ptrdiff_t The effective length sent, or negative if failed
         */
        ptrdiff_t send(const char* data, const size_t len) const;

        /**
         * @brief Receive a block of data through the socket
         * 
         * @param data The pointer to store the data coming
         * @param len The maximum size of the data to store
         * @return ptrdiff_t The effective length received, or negative if failed
         */
        ptrdiff_t recv(char* data, const size_t len) const;

        /**
         * @brief Just like recv, but it targets the len as required.
         * 
         * @param data The pointer to store the data coming
         * @param len The size expected to fill to the pointer
         * @return ptrdiff_t The effective length received, or negative if failed
         */
        ptrdiff_t recv_autowait(char* data, const size_t len) const;

        /**
         * @brief Get the size of the data that can be read now
         * 
         * @return size_t The amount of data available in the buffer now
         */
        size_t recv_size() const;

        using Base::BaseSocket::valid;
        using Base::BaseSocket::operator bool;
        using Base::BaseSocket::get_type;
        using Base::BaseSocket::get_family;
        using Base::BaseSocket::get_config;
        using Base::BaseSocket::close;
    private:
        using Base::ClientSocket::ClientSocket;

        friend class TCPHost;
    };

    /**
     * @brief TCP Host class for TCP hosting
     */
    class TCPHost : protected Base::HostSocket {
    public:
        /**
         * @brief Construct a new TCP host object
         * 
         * @param port The port used for connection
         * @param family The family of the host
         */
        TCPHost(uint16_t port, e_family family = e_family::UNSPEC);
        ~TCPHost() override = default;

        /**
         * @brief Waits and accepts new connection
         * 
         * @return TCPClient New client connected or throws on failure
         */
        TCPClient accept() const;

        using Base::BaseSocket::valid;
        using Base::BaseSocket::operator bool;
        using Base::BaseSocket::get_type;
        using Base::BaseSocket::get_family;
        using Base::BaseSocket::get_config;
        using Base::BaseSocket::close;
    };

} // namespace Socket
} // namespace Lunaris