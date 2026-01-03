#pragma once

#include <Lunaris/platform.h>

#include <vector>
#include <string>
#include <memory>

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
        const std::string addr;
        const e_family family;
        const uint16_t port;
    };

    class socket;

    class address_info {
    public:
        class iterator;

        address_info(const address_info&) = delete;
        address_info(address_info&&) = delete;
        void operator=(const address_info&) = delete;
        void operator=(address_info&&) = delete;

        // automatic mapping of hints and such. Passive = host (waits for conn)
        address_info(const char* ipaddr, uint16_t port, e_socktype, bool passive = false);
        // manual with hints (needs family and socktype on hints itself)
        address_info(const char* ipaddr, uint16_t port, const addr_info_t& hints);

        ~address_info();

        int get_error() const;

        address_info& filter(e_family);
        address_info& filter(e_socktype);
        address_info& filter(e_protocol);
        address_info& filter(bool(*fcn)(addr_info_t*));

        size_t size() const;        

        iterator begin();
        iterator end();
        const iterator begin() const;
        const iterator end() const;
    private:
        void _init(const char* ipaddr, uint16_t port, const addr_info_t& hints);

        std::vector<addr_info_t*> m_filtered;
        addr_info_t* m_info = nullptr;
        int m_getaddrinfo_err = 0;
    public:        
        class iterator {
        public:
            socket operator*();
            iterator& operator++(int);
            iterator operator++();
            bool operator==(const iterator&) const;
            bool operator!=(const iterator&) const;
        private:
            iterator(const std::vector<addr_info_t*>&, size_t);

            friend class address_info;

            const std::vector<addr_info_t*>& m_ref;
            size_t m_curr;
        };
    };

    class socket {
    public:
        socket(const socket&) = delete;
        socket(socket&&) = delete;
        void operator=(const socket&) = delete;
        void operator=(socket&&) = delete;

        socket(e_family, e_socktype, e_protocol = e_protocol::UNSPEC);
        socket(int, int, int = 0);

        ~socket();

        void close();


        int setopt(int level, int opt, bool en);
        // level => SOL_*; opt => SO_*;
        int setopt(int level, int opt, const void* val, const socklen_t len);


        int getopt(int level, int opt, int& res) const;

        int get_type() const;
        int get_protocol() const;

        const address_storage& get_from() const;
        const address_storage& get_to() const;

        // flag => like FIONREAD, FIONBIO;
        int ioctl(int flag, u_long mode);


        // works only on TCP, returns 0 if no error
        int listen(int connections);

        // works only on TCP
        socket accept();

        bool valid() const;
        operator bool() const;
    private:
        friend class address_info::iterator;

        // duplicate socket (true) or take ownership (false)?
        socket(socket_t);
        socket(socket_t, std::unique_ptr<address_storage>&&);

        void _fill_missing_address_storage();

        socket_t m_sock;

        std::unique_ptr<address_storage> m_sg_from, m_sg_to;
    };




}
}