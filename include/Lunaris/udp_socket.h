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

    class UDP_Client : public Base::ClientSocket {
    public:
        UDP_Client(const char* address, uint16_t port);
        ~UDP_Client() override = default;
        
        ssize_t send(const char*, const size_t) const;
        ssize_t recv(char*, const size_t) const;
    private:
        using Base::ClientSocket::ClientSocket;

        friend class TCP_Host;
    };

    class UDP_Host : public Base::HostSocket {
    public:        
        enum class multicast_scope : uint8_t {
            interface_local = 0x1,
            link_local      = 0x2,
            site_local      = 0x5,
            global          = 0xE,
        };

        struct package {
            enum class type : uint8_t { none, broadcast, multicast };

            std::unique_ptr<char[]> buffer;
            size_t buffer_len;
            ssize_t recvd;
            // only filled if applicable
            uint16_t mc_group;
            multicast_scope mc_scope;
            type type = type::none;
        };

        class UDP_Connection : protected Base::ClientSocket {
        public:
            ssize_t send(const char*, const size_t) const;
            ssize_t recv(char*, const size_t);

            bool operator==(const UDP_Connection&) const;
            bool operator!=(const UDP_Connection&) const;
        private:
            UDP_Connection(std::unique_ptr<sock_info>&& pre_cfg);

            friend class UDP_Host;

            std::mutex m_drams_mtx;
            std::condition_variable m_dgram_trigger;
            std::deque<package> m_dgrams;
        };

        static constexpr uint16_t multicast_group_mdns = 0x00fb;

        UDP_Host(uint16_t port, e_family family = e_family::UNSPEC);
        ~UDP_Host() override;

        std::shared_ptr<UDP_Connection> accept();

        size_t size() const;
        size_t size_on_queue() const;

        void enable_broadcast_ipv4(bool enable);

        // ttl = time to live, how far it goes. 1 is the smallest, local. 255 is the maximum value.
        void join_multicast(uint16_t group, multicast_scope scope = multicast_scope::link_local, bool join = true, int ttl = 1);

        ssize_t send_broadcast(const char* data, size_t len);
        ssize_t send_multicast(const char* data, size_t len, uint16_t group, multicast_scope scope = multicast_scope::link_local);

    private:    
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