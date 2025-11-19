#pragma once

#include <stdint.h>
#include <string>
#include <sstream>
#include <vector>
#include <memory>

#include <Lunaris/types.h>

namespace Lunaris {
namespace Socket {

    class DataFrame {
    public:
        DataFrame() = default;

        DataFrame& operator<<(std::string);
        DataFrame& operator<<(uint16_t);
        DataFrame& operator<<(uint32_t);
        DataFrame& operator<<(uint64_t);
        DataFrame& operator<<(int16_t);
        DataFrame& operator<<(int32_t);
        DataFrame& operator<<(int64_t);
        DataFrame& operator<<(float);
        DataFrame& operator<<(double);

        DataFrame& operator>>(std::string&);
        DataFrame& operator>>(uint16_t&);
        DataFrame& operator>>(uint32_t&);
        DataFrame& operator>>(uint64_t&);
        DataFrame& operator>>(int16_t&);
        DataFrame& operator>>(int32_t&);
        DataFrame& operator>>(int64_t&);
        DataFrame& operator>>(float&);
        DataFrame& operator>>(double&);

        const unsigned char* data() const;
        size_t size() const;
        void set(const unsigned char*, const size_t);
    private:
        std::vector<unsigned char> m_mem;
        size_t m_read_off = 0;

        void _push(const void*, const size_t);
        void _read(void*, const size_t);
    };


    class Client {
    public:
        virtual ~Client() = default;
        virtual size_t send(const DataFrame&) = 0;
        virtual size_t recv(DataFrame&, unsigned) = 0;
    };

    using _OG_Client = ::Lunaris::Socket::Client;

    class Server {
    public:
        virtual ~Server() = default;
        virtual std::unique_ptr<_OG_Client> listen(unsigned) = 0;
    };

    class __NonCpyNonMov {
        __NonCpyNonMov(__NonCpyNonMov&&) = delete;
        void operator=(__NonCpyNonMov&&) = delete;
        __NonCpyNonMov(const __NonCpyNonMov&) = delete;
        void operator=(const __NonCpyNonMov&) = delete;
    public:
        __NonCpyNonMov() = default;
    };


    namespace TCP {

        class Server;

        class Client : public ::Lunaris::Socket::Client, public ::Lunaris::Socket::__NonCpyNonMov {
        public:
            Client(std::string, uint16_t);
            ~Client();

            size_t send(const DataFrame&) override;
            size_t recv(DataFrame&, unsigned = 0) override;
        private:
            friend class ::Lunaris::Socket::TCP::Server;

            socket_t m_sock;
        };

        class Server : public ::Lunaris::Socket::Server, public ::Lunaris::Socket::__NonCpyNonMov {
        public:
            Server(uint16_t);
            ~Server();

            std::unique_ptr<_OG_Client> listen(unsigned) override;
        private:
            std::vector<socket_t> m_socks;
        };
    };


    namespace UDP {

        class Server;

        class Client : public ::Lunaris::Socket::Client, public ::Lunaris::Socket::__NonCpyNonMov {
        public:
            Client(std::string, uint16_t);
            ~Client();

            size_t send(const DataFrame&) override;
            size_t recv(DataFrame&, unsigned = 0) override;
        private:
            friend class ::Lunaris::Socket::UDP::Server;

            socket_t m_sock;
        };

        class Server : public ::Lunaris::Socket::Server, public ::Lunaris::Socket::__NonCpyNonMov {
        public:
            Server(uint16_t);
            ~Server();

            std::unique_ptr<_OG_Client> listen(unsigned) override;
        private:
            std::vector<socket_t> m_socks;
        };
    };
}
}