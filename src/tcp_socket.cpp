#include <Lunaris/socket.h>

namespace Lunaris {
namespace Socket {
namespace TCP {

    Client::Client(std::string ip, uint16_t port)
    {}

    Client::~Client()
    {}

    size_t Client::send(const DataFrame& df)
    {
        return 0;
    }

    size_t Client::recv(DataFrame& df, unsigned u)
    {
        return 0;
    }

    Server::Server(uint16_t port)
    {}

    Server::~Server()
    {}

    Client* Server::listen(unsigned for_ms)
    {
        return nullptr;
    }

    void Server::close()
    {

    }
}
}
}