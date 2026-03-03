#pragma once

#include <vector>
#include <deque>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <shared_mutex>

#include <Lunaris/base_socket.h>

namespace Lunaris {
namespace Socket {

    enum class multicast_scope : uint8_t {
        interface_local = 0x1,
        link_local      = 0x2,
        site_local      = 0x5,
        global          = 0xE,
    };

    class UDP_Client : protected Base::ClientSocket {
    public:
        UDP_Client(const char* address, uint16_t port);
        UDP_Client(uint16_t port);
        ~UDP_Client() override = default;
        
        ptrdiff_t send(const char*, const size_t) const;
        ptrdiff_t recv(char*, const size_t) const;

        void enable_broadcast_ipv4(bool enable);

        // ttl = time to live, how far it goes. 1 is the smallest, local. 255 is the maximum value.
        void join_multicast(uint16_t group, multicast_scope scope = multicast_scope::link_local, bool join = true, int ttl = 1);

        /*ptrdiff_t send_broadcast(const char* data, size_t len);
        ptrdiff_t send_multicast(const char* data, size_t len, uint16_t group, multicast_scope scope = multicast_scope::link_local);*/

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

    class UDP_Broadcaster : protected Base::HostSocket { // It may be a good idea for this to be child of UDP_Host to be able to accept whoever responds.
    public:
        // multicast mode
        UDP_Broadcaster(uint16_t port, uint16_t group, e_family family, multicast_scope scope = multicast_scope::link_local, int ttl = 1);
        // broadcast mode
        UDP_Broadcaster(uint16_t port);

        ptrdiff_t send(const char*, const size_t) const;

        using Base::BaseSocket::valid;
        using Base::BaseSocket::operator bool;
        using Base::BaseSocket::get_type;
        using Base::BaseSocket::get_family;
        using Base::BaseSocket::get_config;
        using Base::BaseSocket::close;
    private:
        const uint16_t m_gid = 0;
        const multicast_scope m_scope = multicast_scope::link_local;
        const bool is_broadcast;
    };

    class UDP_Host : protected Base::HostSocket {
    public:
        struct package {
            enum class type : uint8_t { none, broadcast, multicast };

            std::unique_ptr<char[]> buffer;
            size_t buffer_len;
            ptrdiff_t recvd;
            // only filled if applicable
            uint16_t mc_group;
            multicast_scope mc_scope;
            type type = type::none;
        };

        class UDP_Connection : protected Base::ClientSocket {
        public:
            ptrdiff_t send(const char*, const size_t) const;
            ptrdiff_t recv(char*, const size_t);

            bool operator==(const UDP_Connection&) const;
            bool operator!=(const UDP_Connection&) const;

            using Base::BaseSocket::valid;
            using Base::BaseSocket::operator bool;
            using Base::BaseSocket::get_type;
            using Base::BaseSocket::get_family;
            using Base::BaseSocket::get_config;
            using Base::BaseSocket::close;
        private:
            UDP_Connection(std::unique_ptr<sock_info>&& pre_cfg);

            friend class UDP_Host;

            std::mutex m_drams_mtx;
            std::condition_variable m_dgram_trigger;
            std::deque<package> m_dgrams;
        };

        UDP_Host(uint16_t port, e_family family = e_family::UNSPEC);
        ~UDP_Host() override;

        std::shared_ptr<UDP_Connection> accept();

        size_t size() const;
        size_t size_on_queue() const;

        void enable_broadcast_ipv4(bool enable);

        // ttl = time to live, how far it goes. 1 is the smallest, local. 255 is the maximum value.
        void join_multicast(uint16_t group, multicast_scope scope = multicast_scope::link_local, bool join = true, int ttl = 1);
        
        using Base::BaseSocket::valid;
        using Base::BaseSocket::operator bool;
        using Base::BaseSocket::get_type;
        using Base::BaseSocket::get_family;
        using Base::BaseSocket::get_config;
        using Base::BaseSocket::close;
    protected:
        void async_recv();

        void clear_weak_conns();
        
        std::thread m_async_recv;
        std::atomic_bool m_async_stop = false;
        std::shared_mutex m_conns_mtx;
        std::condition_variable m_wait_for_new_connection;
        std::deque<std::shared_ptr<UDP_Connection>> m_waiting_conns;
        std::vector<std::weak_ptr<UDP_Connection>> m_accepted_conns;
    };


} // namespace Socket
} // namespace Lunaris