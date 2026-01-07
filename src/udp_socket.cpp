#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <string.h>
#include <functional>

#include <Lunaris/udp_socket.h>
#include <Lunaris/exception.h>
#include <Lunaris/debugging.h>

namespace Lunaris {
namespace Socket {

#pragma region Local tools hidden

    extern e_family _get_family_from_ip_addr(const char* ip);

    constexpr uint16_t multicast_group_mdns = 0x00fb;

    void auto_wait_cond(std::condition_variable& cond, unsigned long ms, std::function<bool()> pred)
    {
        std::mutex _w;
        std::unique_lock<std::mutex> _l(_w);

        while (!cond.wait_for(_l, std::chrono::milliseconds(ms), pred));
    }

    constexpr in6_addr make_v6_multicast(multicast_scope scope, uint16_t group)
    {
        in6_addr addr{};
        addr.s6_addr[0] = 0xff;
        addr.s6_addr[1] = static_cast<uint8_t>(scope);
        addr.s6_addr[14] = group >> 8;
        addr.s6_addr[15] = group & 0xff;
        return addr;
    };

    constexpr in_addr make_v4_multicast(uint16_t group)
    {
        in_addr addr{};
        char format[16];
        snprintf(format, 16, "239.255.%hu.%hu", group >> 8, group & 0xff);
        inet_pton(AF_INET, format, &addr);
        return addr;
    };

    constexpr void extract_scope_and_group_from_msghdr(msghdr& msg, UDP_Host::package& pkg) {
        for (cmsghdr* cmsg = CMSG_FIRSTHDR(&msg); cmsg; cmsg = CMSG_NXTHDR(&msg, cmsg))
        {
            if (cmsg->cmsg_level == IPPROTO_IPV6 &&
                cmsg->cmsg_type  == IPV6_PKTINFO)
            {
                auto* iinfo =
                    reinterpret_cast<const in6_pktinfo*>(CMSG_DATA(cmsg));

                const in6_addr& dst = iinfo->ipi6_addr;

                if (!IN6_IS_ADDR_MULTICAST(&dst))
                    continue;

                pkg.type = UDP_Host::package::type::multicast;
                pkg.mc_scope = static_cast<multicast_scope>(dst.s6_addr[1] & 0x0f);
                pkg.mc_group =
                    (static_cast<uint16_t>(dst.s6_addr[14]) << 8) |
                    static_cast<uint16_t>(dst.s6_addr[15]);
                
                    break;
            }

            if (cmsg->cmsg_level == IPPROTO_IP &&
                cmsg->cmsg_type  == IP_PKTINFO)
            {
                auto* iinfo =
                    reinterpret_cast<const in_pktinfo*>(CMSG_DATA(cmsg));

                uint32_t dst = ntohl(iinfo->ipi_addr.s_addr);

                if (dst == 0xffffffff) {
                    pkg.type = UDP_Host::package::type::broadcast;
                    pkg.mc_scope = multicast_scope::site_local;
                    break;
                }

                if ((dst & 0xf0000000) == 0xe0000000) {
                    pkg.type = UDP_Host::package::type::multicast;
                    pkg.mc_scope = multicast_scope::site_local;
                    pkg.mc_group =
                        static_cast<uint16_t>(
                            ((dst >> 8) & 0xff00) |
                            (dst & 0xff)
                        );                            
                    break;
                }
            }
        }
    };

    constexpr void enable_broadcast_on(socket_t sock, bool enable) {
        int yes = enable ? 1 : 0;
        if (::setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &yes, sizeof(yes)) != 0) {
            throw socket_exception("Broadcast error - cannot toggle broadcast");
        }

        yes = 1;
        if (::setsockopt(sock, IPPROTO_IP, IP_PKTINFO, &yes, sizeof(yes)) != 0) {
            throw socket_exception("Broadcast error - cannot set packet info");
        }
    }
    
    constexpr void join_multicast_on(socket_t sock, e_family family, uint16_t gid, multicast_scope scope, bool join, int ttl)
    {
        if (gid == 0) {
            throw socket_exception("Multicast error - invalid group id. It cannot be zero.");
        }

        if (ttl <= 0) ttl = 1;
        if (ttl > 255) ttl = 255;

        if (/*!m_sock || */family == e_family::UNSPEC/* || m_sock->type != e_socktype::DGRAM*/)
            throw socket_exception("Multicast error - you cannot enable multicast on dual (UNSPEC) or non DGRAM socket.");

        if (family == e_family::IPV4) {
            if (scope != multicast_scope::link_local)
                throw socket_exception("Multicast error - IPV4 only supports LINK_LOCAL as option for multicast scope.");

            ip_mreq mreq{};
            mreq.imr_interface.s_addr = INADDR_ANY;

            switch (gid) {
                case multicast_group_mdns:
                    inet_pton(AF_INET, "224.0.0.251", &mreq.imr_multiaddr);
                    break;
                default:
                    mreq.imr_multiaddr = make_v4_multicast(gid);
                    break;
            }
            {
                int on = 1;
                if (::setsockopt(sock, IPPROTO_IP, IP_PKTINFO, &on, sizeof(on)) != 0) {
                    throw socket_exception("Multicast error - cannot enable packet info");
                }
            }
            if (::setsockopt(sock, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl)) != 0) {
                throw socket_exception("Multicast error - cannot set TTL");
            }

            int opt = join ? IP_ADD_MEMBERSHIP : IP_DROP_MEMBERSHIP;
            if (::setsockopt(
                sock,
                IPPROTO_IP,
                opt,
                &mreq,
                sizeof(mreq)
            ) != 0) {
                throw socket_exception("Multicast error - cannot add/drop multicast membership");
            }

            return;
        }

        if (family == e_family::IPV6) {
            ipv6_mreq mreq{};
            // safe zone: ff02::[0x1000..0x7fff]. Technically safe from 0x0001..0x7fff
            mreq.ipv6mr_multiaddr = make_v6_multicast(scope, gid);
            mreq.ipv6mr_interface = 0; // let the kernel decide

            {
                int on = 1;
                if (::setsockopt(sock, IPPROTO_IPV6, IPV6_RECVPKTINFO, &on, sizeof(on)) != 0) {
                    throw socket_exception("Multicast error - cannot enable packet info");
                }
            }
            
            if (::setsockopt(sock, IPPROTO_IPV6, IPV6_MULTICAST_HOPS, &ttl, sizeof(ttl)) != 0) {
                throw socket_exception("Multicast error - cannot set TTL");
            }

            int opt = join ? IPV6_JOIN_GROUP : IPV6_LEAVE_GROUP;
            if (::setsockopt(
                sock,
                IPPROTO_IPV6,
                opt,
                &mreq,
                sizeof(mreq)
            ) != 0) {
                throw socket_exception("Multicast error - cannot add/drop multicast group");
            }

            return;
        }
    }

#pragma endregion

#pragma region UDP Area

    UDP_Client::UDP_Client(const char* address, uint16_t port)
        : ClientSocket(address, port, e_socktype::DGRAM)
    {}

    UDP_Client::UDP_Client(uint16_t port)
        : ClientSocket(nullptr, port, e_socktype::DGRAM)
    {}

    ssize_t UDP_Client::send(const char* data, const size_t len) const
    {
        return ::sendto(m_sock->sock, data, len, 0, (addr_t*)&m_sock->storage, m_sock->storage_len);
    }

    ssize_t UDP_Client::recv(char* data, const size_t len) const
    {
        addr_storage_t from{};
        socklen_t from_len = sizeof(addr_storage_t);
        ssize_t res;

        do {
            res = ::recvfrom(m_sock->sock, data, len, 0, (addr_t*)&from, &from_len);
            if (res <= 0) return res;
        } while (from_len != m_sock->storage_len || ::memcmp(&m_sock->storage, &from, from_len) != 0);

        return res;
    }

    void UDP_Client::enable_broadcast_ipv4(bool enable)
    {
        if (!m_sock || get_family() != e_family::IPV4)
            throw socket_exception("Broadcast error - invalid setup: not IPV4 only or empty socket");

        enable_broadcast_on(m_sock->sock, enable);
    }
    
    void UDP_Client::join_multicast(uint16_t gid, multicast_scope scope, bool join, int ttl)
    {
        const auto current_family = get_family();

        if (!m_sock || current_family == e_family::UNSPEC || m_sock->type != e_socktype::DGRAM)
            throw socket_exception("Multicast error - you cannot enable multicast on dual (UNSPEC) or non DGRAM socket.");

        join_multicast_on(m_sock->sock, current_family, gid, scope, join, ttl);
    }
    
    
    UDP_Broadcaster::UDP_Broadcaster(uint16_t port, uint16_t group, e_family family, multicast_scope scope, int ttl)
        : HostSocket(port, family, e_socktype::DGRAM), m_gid(group), m_scope(scope), is_broadcast(false)
    {
        if (family == e_family::UNSPEC) {
            m_sock.reset();
            throw socket_exception("Invalid configuration on UDP_Broadcaster - family must be defined: IPV4 or IPV6, not UNSPEC!");
        }

        join_multicast_on(m_sock->sock, family, group, scope, true, ttl);
    }

    UDP_Broadcaster::UDP_Broadcaster(uint16_t port)
        : HostSocket(port, e_family::IPV4, e_socktype::DGRAM), is_broadcast(true)
    {
        enable_broadcast_on(m_sock->sock, true);
    }

    ssize_t UDP_Broadcaster::send(const char* data, const size_t len) const
    {
        if (is_broadcast) {
            const auto current_cfg = get_config();
            _lunaris_socket_debug_c("BROADCASTING DATA IPV4 SIZE={} PORT={}", len, current_cfg.port);

            sockaddr_in addr{};
            addr.sin_family = AF_INET;
            addr.sin_port   = htons(current_cfg.port);
            addr.sin_addr.s_addr = htonl(INADDR_BROADCAST);

            return ::sendto(
                m_sock->sock,
                data,
                len,
                0,
                reinterpret_cast<sockaddr*>(&addr),
                sizeof(addr)
            );
        }
        else {
            const auto current_family = get_family();
            const auto current_cfg = get_config();

            if (!m_sock || current_family == e_family::UNSPEC || m_sock->type != e_socktype::DGRAM) {
                _lunaris_socket_debug_c("MULTICAST NOT ALLOWED ON UDP_BROADCAST!");
                return -1;
            }

            if (current_family == e_family::IPV4) {
                _lunaris_socket_debug_c("MULTICAST DATA IPV4 SIZE={} PORT={} GROUP={}", len, current_cfg.port, m_gid);
                sockaddr_in addr{};
                addr.sin_family = AF_INET;
                addr.sin_port   = htons(current_cfg.port);
                addr.sin_addr   = make_v4_multicast(m_gid);

                return ::sendto(
                    m_sock->sock,
                    data,
                    len,
                    0,
                    reinterpret_cast<sockaddr*>(&addr),
                    sizeof(addr)
                );
            }
            
            if (current_family == e_family::IPV6) {
                _lunaris_socket_debug_c("MULTICAST DATA IPV4 SIZE={} PORT={} GROUP={} SCOPE={}", len, current_cfg.port, m_gid, (int)m_scope);
                sockaddr_in6 addr{};
                addr.sin6_family = AF_INET6;
                addr.sin6_port   = htons(current_cfg.port);
                addr.sin6_addr   = make_v6_multicast(m_scope, m_gid);

                return ::sendto(
                    m_sock->sock,
                    data,
                    len,
                    0,
                    reinterpret_cast<sockaddr*>(&addr),
                    sizeof(addr)
                );
            }
        }

        return -1;
    }


    ssize_t UDP_Host::UDP_Connection::send(const char* data, const size_t len) const
    {
        return ::sendto(m_sock->sock, data, len, 0, (addr_t*)&m_sock->storage, m_sock->storage_len);
    }

    ssize_t UDP_Host::UDP_Connection::recv(char* data, const size_t len)
    {
        if (m_dgrams.size() == 0) 
            auto_wait_cond(m_dgram_trigger, 50, [this]{ return m_dgrams.size() > 0; });
        
        std::lock_guard<std::mutex> ld(m_drams_mtx);
        auto pkg = std::move(m_dgrams.front());
        m_dgrams.pop_front();

        ::memcpy(data, pkg.buffer.get(), std::min(pkg.buffer_len, len));
        return pkg.recvd;
    }

    bool UDP_Host::UDP_Connection::operator==(const UDP_Connection& o) const
    {
        return o.m_sock && this->m_sock && (*o.m_sock) == (*this->m_sock);
    }
    bool UDP_Host::UDP_Connection::operator!=(const UDP_Connection& o) const
    {
        return !o.m_sock || !this->m_sock || (*o.m_sock) != (*this->m_sock);
    }

    UDP_Host::UDP_Connection::UDP_Connection(std::unique_ptr<sock_info>&& pre_cfg)
        : Base::ClientSocket(std::move(pre_cfg))
    {}

    UDP_Host::UDP_Host(uint16_t port, e_family family)
        : HostSocket(port, family, e_socktype::DGRAM),
          m_async_recv([this]{ async_recv(); })
    {
    }

    UDP_Host::~UDP_Host() {
        m_async_stop = true;
        m_async_recv.join();

        m_accepted_conns.clear();
        m_waiting_conns.clear();
    }

    std::shared_ptr<UDP_Host::UDP_Connection> UDP_Host::accept()
    {
        if (m_waiting_conns.size() == 0) {
            _lunaris_socket_debug_c("ON WAIT UDP_HOST::ACCEPT");
            auto_wait_cond(m_wait_for_new_connection, 200, [this]{ return m_waiting_conns.size() > 0; });
        }

        std::lock_guard<std::shared_mutex> l(m_conns_mtx);

        auto ptr = std::move(m_waiting_conns.front());
        m_waiting_conns.pop_front();
        m_accepted_conns.push_back(ptr);

        return ptr;
    }

    size_t UDP_Host::size() const 
    {
        return m_waiting_conns.size() + m_accepted_conns.size();
    }

    size_t UDP_Host::size_on_queue() const 
    {
        return m_waiting_conns.size();
    }

    void UDP_Host::enable_broadcast_ipv4(bool enable)
    {
        if (!m_sock || get_family() != e_family::IPV4)
            throw socket_exception("Broadcast error - invalid setup: not IPV4 only or empty socket");

        enable_broadcast_on(m_sock->sock, enable);
    }
    
    void UDP_Host::join_multicast(uint16_t gid, multicast_scope scope, bool join, int ttl)
    {
        const auto current_family = get_family();

        if (!m_sock || current_family == e_family::UNSPEC || m_sock->type != e_socktype::DGRAM)
            throw socket_exception("Multicast error - you cannot enable multicast on dual (UNSPEC) or non DGRAM socket.");

        join_multicast_on(m_sock->sock, current_family, gid, scope, join, ttl);
    }

    void UDP_Host::async_recv()
    {
        const auto check_sock_has_in = [](socket_t s) -> bool {
            constexpr unsigned long nfds = 1;
            poll_t poll[nfds];

            ::memset(poll, 0, sizeof(poll_t) * nfds);
            poll[0].events = POLLIN;
            poll[0].fd = s;

            if (platform::pollsocket(poll, nfds, 100) < 0)
                return false;
            
            return (poll[0].revents & POLLIN) > 0;
        };

        if (!m_sock) { // UDP host failed to instantiate itself
            _lunaris_socket_debug_c("UDP_HOST MALFORMED START, EMPTY SOCK. DROP.");
            return;
        }

        bool flag_had_expired_ptr = false;

        const auto get_next_package_size = [](socket_t s) -> int {
            int res = 0;
            if (platform::ioctlsocket(s, FIONREAD, res) < 0) return -1;
            return res;
        };
        const auto cleanup_weak_ptrs = [&] {
            flag_had_expired_ptr = false;
            clear_weak_conns();
        };

        while(!m_async_stop.load()) {
            if (flag_had_expired_ptr) cleanup_weak_ptrs();
            if (!check_sock_has_in(m_sock->sock)) continue; // less CPU aggressive

            const int buf_next = get_next_package_size(m_sock->sock);
            if (buf_next <= 0) {
                continue;
            }

            _lunaris_socket_debug_c("UDP_HOST GOT PACKAGE EXPECTED_SIZE={}", buf_next);

            auto info = m_sock->make_ref();
            package pkg = {
                .buffer = std::unique_ptr<char[]>(new char[buf_next]),
                .buffer_len = static_cast<size_t>(buf_next)
            };
            
            iovec iov{};
            iov.iov_base = pkg.buffer.get();
            iov.iov_len  = buf_next;

            char cmsgbuf[CMSG_SPACE(sizeof(in6_pktinfo)) + CMSG_SPACE(sizeof(in_pktinfo))];

            msghdr msg{};
            msg.msg_name       = &info->storage;
            msg.msg_namelen    = info->storage_len;
            msg.msg_iov        = &iov;
            msg.msg_iovlen     = 1;
            msg.msg_control    = cmsgbuf;
            msg.msg_controllen = sizeof(cmsgbuf);

            pkg.recvd = ::recvmsg(info->sock, &msg, 0);

            if (pkg.recvd < 1) {
                _lunaris_socket_debug_c("UDP_HOST DROPPED PACKAGE, FAILED");
                continue;
            }

            extract_scope_and_group_from_msghdr(msg, pkg);

            if (pkg.type == package::type::none) {
                _lunaris_socket_debug_c("UDP_HOST GOT PACKAGE TYPE: DEFAULT");
            }
            else if (pkg.type == package::type::broadcast) {
                _lunaris_socket_debug_c("UDP_HOST GOT PACKAGE TYPE: BROADCAST");
            }
            else {
                _lunaris_socket_debug_c("UDP_HOST GOT PACKAGE TYPE: MULTICAST GROUP={} SCOPE={}", pkg.mc_group, (int)pkg.mc_scope);
            }

            { // First try to find target
                bool found = false;
                std::shared_lock<std::shared_mutex> l(m_conns_mtx);

                for (auto& i : m_waiting_conns) {
                    if (*i->m_sock == *info) {
                        found = true;
                        std::lock_guard<std::mutex> ld(i->m_drams_mtx);
                        i->m_dgrams.push_back(std::move(pkg));
                        i->m_dgram_trigger.notify_one();
                        break;
                    }
                }
                if (found) {
                    _lunaris_socket_debug_c("REDIRECTED TO WAITING CONNS");
                    continue;
                }

                for (auto& i : m_accepted_conns) {
                    auto ptr = i.lock(); // guarantee it exists, if it does
                    _lunaris_socket_debug_c("### TEST NULL={} EXPIRED={}", ptr.get() == nullptr ? "true" : "false", i.expired() ? "true" : "false");
                    if (!ptr) {
                        flag_had_expired_ptr = true;
                        continue;
                    }
                    if (*ptr->m_sock == *info) {
                        found = true;
                        std::lock_guard<std::mutex> ld(ptr->m_drams_mtx);
                        ptr->m_dgrams.push_back(std::move(pkg));
                        ptr->m_dgram_trigger.notify_one();
                        break;
                    }
                }
                if (found) {
                    _lunaris_socket_debug_c("REDIRECTED TO CURRENT CONNS");
                    continue;
                }
            }
            // Second, if it doesn't find it, it is a new target!

            auto con = std::shared_ptr<UDP_Connection>(new UDP_Connection(std::move(info)));
            con->m_dgrams.push_back(std::move(pkg));

            _lunaris_socket_debug_c("UDP_HOST GOT NEW CONN!");
            std::unique_lock<std::shared_mutex> l(m_conns_mtx);
            m_waiting_conns.push_back(std::move(con));
            m_wait_for_new_connection.notify_one();
        }
    }

    void UDP_Host::clear_weak_conns()
    {
        std::unique_lock<std::shared_mutex> l(m_conns_mtx);
        for(auto i = m_accepted_conns.begin(); i != m_accepted_conns.end();) {
            if (i->expired()) i = m_accepted_conns.erase(i);
            else ++i;
        }
    }

#pragma endregion

} // namespace Socket
} // namespace Lunaris