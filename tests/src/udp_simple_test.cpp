//#include "hardcoded.h"

#include <print>
#include <thread>

#include <Lunaris/socket.h>

#include "common.h"

using namespace Lunaris::Socket;

int main() {
    constexpr char sel_path[] = "localhost";
    const uint16_t sel_port = generate_random_port();

    const std::string hello_trigger = "hi";

    std::println("=== > Initializing integration test with path '{}' and random port selected '{}' < ===", sel_path, sel_port);

    std::print("host>");
    UDP_Host host(sel_port);

    std::print("client>");
    UDP_Client cli(sel_path, sel_port);

    std::print("gen_strings>");
    const std::string blocks[] = {
        generate_random_string(20),
        generate_random_string(50),
        generate_random_string(200)
    };

    /*std::println("Testing random strings:");
    for(auto& i : blocks) std::println("- '{}'", i);*/

    std::print("trigger>");
    cli.send(hello_trigger.data(), hello_trigger.size());

    std::print("accept>");
    auto hst_ptr = host.accept();
    if (!hst_ptr) throw std::runtime_error("Accept resulted in nullptr.");
    
    auto& hst = *hst_ptr;

    std::print("check_hello>");
    if (!recv_udp_and_check_auto(hst, hello_trigger)) throw std::runtime_error("Hello pseudo handshake failed!");

    std::print("send_all_cli>");
    for(auto& i : blocks) if (!send_and_check_auto(cli, i)) throw std::runtime_error("Failed to send from client once! String: '" + i + "'");

    std::print("recv_all_hst>");
    for(auto& i : blocks) if (!recv_udp_and_check_auto(hst, i)) throw std::runtime_error("Failed on recv of string '" + i + "'.");

    std::print("send_all_hst>");
    for(auto& i : blocks) if (!send_and_check_auto(hst, i)) throw std::runtime_error("Failed to send from client once! String: '" + i + "'");

    std::print("recv_all_cli>");
    for(auto& i : blocks) if (!recv_udp_and_check_auto(cli, i)) throw std::runtime_error("Failed on recv of string '" + i + "'.");

    std::println("\n=== > PASSED! < ===");
}
