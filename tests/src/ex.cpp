#include <iostream>
#include <Lunaris/socket.h>

void as_host();
void as_client();

constexpr uint16_t port = 52269;

using namespace Lunaris::Socket;

int main()
{
    std::cout << "Hello world\n";

    as_client();


    return 0;
}


void as_host()
{
    auto info = address_info("localhost", port, e_socktype::STREAM, true);
    
    info.filter(e_family::IPV4);

    for(auto i : info) {
        socket sock = *i;
        

    }
}

void as_client()
{
    auto info = address_info("localhost", port, e_socktype::STREAM, false);

    info.filter(e_family::IPV4);


}