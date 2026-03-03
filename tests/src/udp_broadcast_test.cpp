#include <print>
#include <thread>

#include <Lunaris/socket.h>

#include "common.h"

using namespace Lunaris::Socket;

int main() {
    const uint16_t sel_port = generate_random_port();
    const auto broadcasting_msg = generate_random_string(150);
    const auto second_msg = generate_random_string(500);

    std::println("=== > Initializing integration test with random port selected '{}' < ===", sel_port);


    std::print("host>");
    UDP_Host host(sel_port, e_family::IPV4);

    std::print("broadcaster>");
    UDP_Broadcaster cli(sel_port);

    std::print("host_bc_on>");
    host.enable_broadcast_ipv4(true);

    std::print("bc_send>");
    if (!send_and_check_auto(cli, broadcasting_msg)) throw std::runtime_error("Failed to send from client once! String: '" + broadcasting_msg + "'");

    std::print("accept>");
    auto hst_ptr = host.accept();
    if (!hst_ptr) throw std::runtime_error("Accept resulted in nullptr.");

    auto& hst = *hst_ptr;

    std::print("bc_recv>");
    if (!recv_udp_and_check_auto(hst, broadcasting_msg)) throw std::runtime_error("Failed on recv of broadcasted message.");

    std::print("bc_send2>");
    if (!send_and_check_auto(cli, second_msg)) throw std::runtime_error("Failed to send2 from client once! String: '" + second_msg + "'");

    std::print("bc_recv2>");
    if (!recv_udp_and_check_auto(hst, second_msg)) throw std::runtime_error("Failed on recv2 of broadcasted message.");

    std::println("\n=== > PASSED! < ===");
}
