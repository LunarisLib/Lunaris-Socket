#include <iostream>
#include <thread>

#include <Lunaris/socket.h>

#include "common.h"

using namespace Lunaris::Socket;

int main() {
    const uint16_t sel_port = generate_random_port();
    const uint16_t sel_group = 1234;
    const multicast_scope sel_scope = multicast_scope::link_local;
    const auto broadcasting_msg = generate_random_string(150);
    const auto second_msg = generate_random_string(500);

    std::cout << "=== > Initializing integration test with random port selected '" << sel_port << "' < ===\n";

    std::cout << "host>";
    UDP_Host host(sel_port, e_family::IPV6);

    std::cout << "multicast>";
    UDP_Broadcaster cli(sel_port, sel_group, e_family::IPV6, sel_scope);

    std::cout << "host_mc_on>";
    host.join_multicast(sel_group, sel_scope, true);

    std::cout << "mc_send>";
    if (!send_and_check_auto(cli, broadcasting_msg)) throw std::runtime_error("Failed to send from client once! String: '" + broadcasting_msg + "'");

    std::cout << "accept>";
    auto hst_ptr = host.accept();
    if (!hst_ptr) throw std::runtime_error("Accept resulted in nullptr.");

    auto& hst = *hst_ptr;

    std::cout << "bc_recv>";
    if (!recv_udp_and_check_auto(hst, broadcasting_msg)) throw std::runtime_error("Failed on recv of broadcasted message.");

    std::cout << "bc_send2>";
    if (!send_and_check_auto(cli, second_msg)) throw std::runtime_error("Failed to send2 from client once! String: '" + second_msg + "'");

    std::cout << "bc_recv2>";
    if (!recv_udp_and_check_auto(hst, second_msg)) throw std::runtime_error("Failed on recv2 of broadcasted message.");

    std::cout << "\n=== > PASSED! < ===\n";
}
