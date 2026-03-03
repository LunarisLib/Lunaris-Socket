//#include "hardcoded.h"

#include <print>
#include <thread>

#include <Lunaris/socket.h>

#include "common.h"

using namespace Lunaris::Socket;

int main() {
    constexpr char sel_path[] = "localhost";
    const uint16_t sel_port = generate_random_port();

    std::println("=== > Initializing integration test with path '{}' and random port selected '{}' < ===", sel_path, sel_port);

    std::print("host>");
    TCP_Host host(sel_port);

    std::print("client>");
    TCP_Client cli(sel_path, sel_port);

    std::print("gen_strings>");
    const std::string blocks[] = {
        generate_random_string(20),
        generate_random_string(50),
        generate_random_string(200)
    };

    /*std::println("Testing random strings:");
    for(auto& i : blocks) std::println("- '{}'", i);*/

    std::print("accept>");
    TCP_Client hst = host.accept();
    if (!hst) throw std::runtime_error("Accept resulted in invalid client.");

    std::print("send_all_cli>");
    for(auto& i : blocks) if (!send_and_check_auto(cli, i)) throw std::runtime_error("Failed to send from client once! String: '" + i + "'");

    std::print("recv_all_hst>");
    for(auto& i : blocks) if (!recv_tcp_and_check_auto(hst, i)) throw std::runtime_error("Failed on recv of string '" + i + "'.");

    std::print("send_all_hst>");
    for(auto& i : blocks) if (!send_and_check_auto(hst, i)) throw std::runtime_error("Failed to send from client once! String: '" + i + "'");

    std::print("recv_all_cli>");
    for(auto& i : blocks) if (!recv_tcp_and_check_auto(cli, i)) throw std::runtime_error("Failed on recv of string '" + i + "'.");

    std::println("\n=== > PASSED! < ===");
}
