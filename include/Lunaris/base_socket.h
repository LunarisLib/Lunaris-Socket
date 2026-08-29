#pragma once

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#include <mswsock.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#endif

#include <memory>
#include <string>
#include <cstdint>

#include <Lunaris/platform.h>

namespace Lunaris {
namespace Socket {

    /**
     * @brief Enum that defines the family you want to work with, or any family
     */
    enum class e_family {
        IPV4 = PF_INET,
        IPV6 = PF_INET6,
        UNSPEC = PF_UNSPEC
    };

    /**
     * @brief Enum that defines the type of socket
     */
    enum class e_socktype {
        STREAM = SOCK_STREAM,
        DGRAM = SOCK_DGRAM,
    };

    /**
     * @brief Enum that defined the protocol you'll be using
     */
    enum class e_protocol {
        UNSPEC = 0,
        TCP = IPPROTO_TCP,
        UDP = IPPROTO_UDP
    };

    /**
     * @brief Useful address storage struct for connection information
     */
    struct address_storage {
        std::string address;
        e_family family;
        uint16_t port;
    };

    namespace Base {

        /**
         * @brief Abstract class that encapsulates the common socket operations
         */
        class BaseSocket {
        public:
            virtual ~BaseSocket() = default;

            /**
             * @brief Checks if the socket is a valid one
             * 
             * @return `bool` true if valid
             */
            bool valid() const;

            /**
             * @brief Equivalent to `valid()`
             * 
             * @return `bool` true if valid
             */
            operator bool() const;

            /**
             * @brief Get information of the socket using getopt equivalent call
             * 
             * @param level The level to get information from, like SOL_SOCKET
             * @param opt The option to get information of
             * @param res An integer to store result
             * @return `int` Returns 0 if success
             */
            int getopt(int level, int opt, int& res) const;

            /**
             * @brief Get the type object of the socket
             * 
             * @return `e_socktype` Type of the socket
             */
            e_socktype get_type() const;

            /**
             * @brief Get the family object of the socket
             * 
             * @return `e_family` Family used
             */
            e_family get_family() const;

            /**
             * @brief Control socket flags using ioctlsocket/ioctl equivalent call
             * 
             * @param flag The flag in question
             * @param mode What mode to control
             * @return `int` Returns 0 if success
             */
            int ioctl(int flag, u_long mode);

            /**
             * @brief Get the configuration of this socket
             * 
             * @return `address_storage` Address information
             */
            address_storage get_config() const;

            /**
             * @brief Close connection and cleanup
             */
            void close();
        protected:
            
            /**
             * @brief Socket concern moved to a sub-struct within BaseSocket so it can be safely instantiated and moved around if needed
             */
            struct sock_info {
                socket_t sock;
                e_socktype type;
                addr_storage_t storage = {};
                socklen_t storage_len = 0;
#ifdef _WIN32
                LPFN_WSARECVMSG wsarecvmsg_fn;
#endif

                sock_info(socket_t, e_socktype, addr_storage_t*, socklen_t);
#ifdef _WIN32
                sock_info(socket_t, e_socktype, addr_storage_t*, socklen_t, LPFN_WSARECVMSG);
#endif
                ~sock_info();

                std::unique_ptr<sock_info> make_ref() const;

                bool operator==(const sock_info&) const;
                bool operator!=(const sock_info&) const;

                sock_info(const sock_info&) = delete;
                sock_info(sock_info&&) = delete;
                void operator=(const sock_info&) = delete;
                void operator=(sock_info&&) = delete;
            private:
                bool owns_socket;      
            };

            std::unique_ptr<sock_info> m_sock;
        };

        class HostSocket;

        class ClientSocket : protected BaseSocket {
        public:
            /**
             * @brief Construct a new Client Socket object
             * 
             * @param address String to an address
             * @param port The port used for connection, technically from 0 to 65535
             * @param type What socket type to use
             */
            ClientSocket(const char* address, uint16_t port, e_socktype type);
            virtual ~ClientSocket() = default;
        protected:
            ClientSocket(std::unique_ptr<sock_info>&& pre_cfg);
        };

        class HostSocket : protected BaseSocket {
        public:
            /**
             * @brief Construct a new Host Socket object
             * 
             * @param port The port used for connection, technically from 0 to 65535
             * @param family What family to use, or any
             * @param type What socket type to use
             */
            HostSocket(uint16_t port, e_family family, e_socktype type);
            virtual ~HostSocket() = default;
        };
    }
    
} // namespace Socket
} // namespace Lunaris