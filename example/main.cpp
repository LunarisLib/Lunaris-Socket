#include <iostream>

#include <Lunaris/socket.h>


using namespace Lunaris::Socket;

int main() {
    std::cout << "Using one of the tests as example here.\n";

    TCPHost host(55366);

    TCPClient cli("localhost", 55366);

    TCPClient hst = host.accept();
    if (!hst) throw std::runtime_error("Accept resulted in invalid client.");


    std::cout << "\n=== > PASSED! < ===\n";

    cli.close();
    hst.close();
    host.close();

    return 0;
}
