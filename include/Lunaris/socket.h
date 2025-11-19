#pragma once

#include <stdint.h>
#include <string>
#include <sstream>
#include <vector>

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
        virtual ~Client() = 0;
        virtual size_t send(DataFrame&&) = 0;
        virtual size_t send(const DataFrame&) = 0;
        virtual size_t recv(DataFrame&, unsigned) = 0;
    };

    class Server {
    public:
        virtual ~Server() = 0;
        virtual Client* listen(unsigned) = 0;
        virtual void close() = 0;
    };

    namespace TCP {

        class Client : public ::Lunaris::Socket::Client {
        public:
            Client(std::string, uint16_t);

            size_t send(DataFrame&&) override;
            size_t send(const DataFrame&) override;
            size_t recv(DataFrame&, unsigned) override;
        };

        class Server : public ::Lunaris::Socket::Server {
        public:
            Server(uint16_t);

            Client* listen(unsigned) override;
            void close() override;
        };
    };

    namespace UDP {

        class Client : public ::Lunaris::Socket::Client {
        public:
            Client(std::string, uint16_t);

            size_t send(DataFrame&&) override;
            size_t send(const DataFrame&) override;
            size_t recv(DataFrame&, unsigned) override;
        };

        class Server : public ::Lunaris::Socket::Server {
        public:
            Server(uint16_t);

            Client* listen(unsigned) override;
            void close() override;
        };
    };
}
}