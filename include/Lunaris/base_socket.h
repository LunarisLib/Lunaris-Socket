#pragma once

#include <sys/socket.h>
#include <netinet/in.h>

#include <memory>
#include <string>

#include <Lunaris/platform.h>

namespace Lunaris {
namespace Socket {

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

    struct address_storage {
        std::string address;
        e_family family;
        uint16_t port;
    };

    namespace Base {

        class BaseSocket {
        public:
            virtual ~BaseSocket() = default;

            bool valid() const;
            operator bool() const;

            int getopt(int level, int opt, int& res) const;
            e_socktype get_type() const;
            e_family get_family() const;
            int ioctl(int flag, u_long mode);

            address_storage get_config() const;

            void close();
        protected:
            // absorbs creations, guarantees socket close
            struct sock_info {
                socket_t sock;
                e_socktype type;
                addr_storage_t storage = {};
                socklen_t storage_len = 0;

                sock_info(socket_t, e_socktype, addr_storage_t*, socklen_t);
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
            ClientSocket(const char* address, uint16_t port, e_socktype type);
            virtual ~ClientSocket() = default;
        protected:
            ClientSocket(std::unique_ptr<sock_info>&& pre_cfg);
        };

        class HostSocket : protected BaseSocket {
        public:
            HostSocket(uint16_t port, e_family family, e_socktype type);
            virtual ~HostSocket() = default;
        };
    }
    
} // namespace Socket
} // namespace Lunaris