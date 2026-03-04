#include <iostream>

#include <Lunaris/socket.h>


using namespace Lunaris::Socket;

int main() {
    std::cout << "Using one of the tests as example here.\n";

    TCP_Host host(55366);

    TCP_Client cli("localhost", 55366);

    TCP_Client hst = host.accept();
    if (!hst) throw std::runtime_error("Accept resulted in invalid client.");


    std::cout << "\n=== > PASSED! < ===\n";

    cli.close();
    hst.close();
    host.close();

    return 0;
}
