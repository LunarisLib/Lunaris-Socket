#include <iostream>
#include <thread>

#include <Lunaris/socket.h>

#include "common.h"

using namespace Lunaris::Socket;

int main() {
    constexpr char sel_path[] = "localhost";
    const uint16_t sel_port = generate_random_port();

    std::cout << "=== > Initializing integration test with path '" << sel_path << "' and random port selected '" << sel_port << "' < ===\n";

    std::cout << "host>";
    TCP_Host host(sel_port);

    std::cout << "client>";
    TCP_Client cli(sel_path, sel_port);

    std::cout << "gen_strings>";
    const std::string blocks[] = {
        generate_random_string(20),
        generate_random_string(50),
        generate_random_string(200)
    };

    std::cout << "accept>";
    TCP_Client hst = host.accept();
    if (!hst) throw std::runtime_error("Accept resulted in invalid client.");

    std::cout << "send_all_cli>";
    for(auto& i : blocks) if (!send_and_check_auto(cli, i)) throw std::runtime_error("Failed to send from client once! String: '" + i + "'");

    std::cout << "recv_all_hst>";
    for(auto& i : blocks) if (!recv_tcp_and_check_auto(hst, i)) throw std::runtime_error("Failed on recv of string '" + i + "'.");

    std::cout << "send_all_hst>";
    for(auto& i : blocks) if (!send_and_check_auto(hst, i)) throw std::runtime_error("Failed to send from client once! String: '" + i + "'");

    std::cout << "recv_all_cli>";
    for(auto& i : blocks) if (!recv_tcp_and_check_auto(cli, i)) throw std::runtime_error("Failed on recv of string '" + i + "'.");

    std::cout << "\n=== > PASSED! < ===\n";
}
