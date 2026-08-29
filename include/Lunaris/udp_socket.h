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

    /**
     * @brief Scope for multicast scenarios
     */
    enum class multicast_scope : uint8_t {
        interface_local = 0x1,
        link_local      = 0x2,
        site_local      = 0x5,
        global          = 0xE,
    };

    /**
     * @brief UDP Client class for UDP connections to servers
     * 
     */
    class UDP_Client : protected Base::ClientSocket {
    public:
        /**
         * @brief Construct a new UDP client object
         * 
         * @param address The address to connect to
         * @param port The port used for connection
         */
        UDP_Client(const char* address, uint16_t port);
        UDP_Client(uint16_t port);
        ~UDP_Client() override = default;
        
        /**
         * @brief Send a dataframe of data through the socket
         * 
         * @param data The data to send
         * @param len The size of the data
         * @return ptrdiff_t The effective length sent, or negative if failed
         */
        ptrdiff_t send(const char* data, const size_t len) const;

        /**
         * @brief Receive a dataframe of data through the socket
         * 
         * @param data The pointer to store the data coming
         * @param len The pointer size / dataframe size
         * @return ptrdiff_t The effective length received, or negative if failed
         */
        ptrdiff_t recv(char* data, const size_t len) const;

        /**
         * @brief Allow socket to listen to broadcast in IPV4 mode
         * 
         * @param enable true connects to broadcast
         */
        void enable_broadcast_ipv4(bool enable);

        /**
         * @brief Joins or leaves a multicast - to listen to
         * 
         * @param group Group ID to connect to
         * @param scope How far it goes in scope (defaults to link local)
         * @param join Are you joining or leaving the multicast?
         * @param ttl Time To Live, how many bounces around should it allow before routers and computers stop broadcasting
         */
        void join_multicast(uint16_t group, multicast_scope scope = multicast_scope::link_local, bool join = true, int ttl = 1);

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

    /**
     * @brief Specialized UDP Host class for broadcasting in multicast or broadcast modes
     */
    class UDP_Broadcaster : protected Base::HostSocket {
    public:
        /**
         * @brief Construct a new UDP broadcaster object in multicast mode
         * 
         * @param port The port used for connection
         * @param group Group ID to broadcast to
         * @param family The family of the host
         * @param scope How far it goes in scope (defaults to link local)
         * @param ttl Time To Live, how many bounces around should it allow before routers and computers stop broadcasting
         */
        UDP_Broadcaster(uint16_t port, uint16_t group, e_family family, multicast_scope scope = multicast_scope::link_local, int ttl = 1);
        
        /**
         * @brief Construct a new UDP broadcaster object in broadcast mode (IPV4)
         * 
         * @param port The port used for connection
         */
        UDP_Broadcaster(uint16_t port);


        /**
         * @brief Send a dataframe of data through the socket
         * 
         * @param data The data to broadcast
         * @param len The size of the data
         * @return ptrdiff_t The effective length broadcasted, or negative if failed
         */
        ptrdiff_t send(const char* data, const size_t len) const;

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

    /**
     * @brief UDP Host class for UDP hosting
     * 
     */
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

        /**
         * @brief A pseudo client class for UDP connections received by UDP Host
         */
        class UDP_Connection : protected Base::ClientSocket {
        public:
            /**
             * @brief Send a dataframe of data through the socket
             * 
             * @param data The data to send
             * @param len The size of the data
             * @return ptrdiff_t The effective length sent, or negative if failed
             */
            ptrdiff_t send(const char* data, const size_t len) const;

            /**
             * @brief Receive a dataframe of data through the socket
             * 
             * @param data The pointer to store the data coming
             * @param len The pointer size / dataframe size
             * @return ptrdiff_t The effective length received, or negative if failed
             */
            ptrdiff_t recv(char* data, const size_t len);

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

        /**
         * @brief Construct a new UDP host object
         * 
         * @param port The port used for connection
         * @param family The family of the host
         */
        UDP_Host(uint16_t port, e_family family = e_family::UNSPEC);
        ~UDP_Host() override;

        /**
         * @brief Attempts to get a pseudo connection. Waits indefinitely
         * 
         * @return std::shared_ptr<UDP_Connection> A pseudo-client to manage received package
         */
        std::shared_ptr<UDP_Connection> accept();

        /**
         * @brief Total size of what is in queue to be accepted + pseudo clients
         * 
         * @return size_t The total amount of clients (accepted and running)
         */
        size_t size() const;

        /**
         * @brief Total size of what is in queue to be accepted only (ready for accept())
         * 
         * @return size_t The total amount of pseudo clients ready to be accepted
         */
        size_t size_on_queue() const;

        /**
         * @brief Allow socket to listen to broadcast in IPV4 mode
         * 
         * @param enable true connects to broadcast
         */
        void enable_broadcast_ipv4(bool enable);

        /**
         * @brief Joins or leaves a multicast - to listen to
         * 
         * @param group Group ID to connect to
         * @param scope How far it goes in scope (defaults to link local)
         * @param join Are you joining or leaving the multicast?
         * @param ttl Time To Live, how many bounces around should it allow before routers and computers stop broadcasting
         */
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