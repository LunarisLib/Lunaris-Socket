#include <print>
#include <thread>
#include <atomic>

#include <Lunaris/socket.h>

#include "common.h"

/*
This is possibly wrongly built because hosting the same port / IP and broadcast to itself may duplicate responses / accept() there and keep references that does not match the thought behavior on a real scenario.
To test this kind of stuff, first we'll need two machines
*/


using namespace Lunaris::Socket;

enum class test_state {
    THR_BEGIN_STATE,
    THR_WAIT_FOR_SIGNAL,
    THR_CAN_INIT,
    THR_WAIT_FOR_CAN_SEND,
    THR_CAN_SEND,
    THR_WAIT_FOR_CAN_REPLY_BACK,
    THR_REPLY_BACK,
    THR_WAIT_SPAM_EACH_OTHER,
    THR_DO_SPAM,
    THR_WAIT_FOR_FINISH,
    THR_FINISHING,
    THR_FINISHED
};

constexpr void wait_until(std::atomic<test_state>& state, test_state target) { while(state.load() != target) std::this_thread::sleep_for(std::chrono::milliseconds(15)); }
constexpr void wait_both_for(std::atomic<test_state>& s1, std::atomic<test_state>& s2, test_state target) { while(s1.load() != target || s2.load() != target) std::this_thread::sleep_for(std::chrono::milliseconds(15)); }
constexpr void wait_both_for(std::atomic<test_state>& s1, std::atomic<test_state>& s2, test_state target, void(*do_while)()) { while(s1.load() != target || s2.load() != target) { if (do_while) do_while(); std::this_thread::sleep_for(std::chrono::milliseconds(15)); } }
void client(std::atomic<test_state>& sstate, bool do_send_first);

const uint16_t sel_port = generate_random_port();
constexpr size_t sel_test_spam_msgs_size = 200;

size_t g_ref_threads[2]{};

int main() {
    std::atomic<test_state> thrs1{test_state::THR_BEGIN_STATE}, thrs2{test_state::THR_BEGIN_STATE};
    std::thread thr1, thr2;

    std::println("=== > Initializing integration test with random port selected '{}' < ===", sel_port);

    std::print("launch_threads>");
    thr1 = std::thread([&] { client(thrs1, true); });
    thr2 = std::thread([&] { client(thrs2, false); });

    std::print("wait_thrs>");
    wait_both_for(thrs1, thrs2, test_state::THR_WAIT_FOR_SIGNAL);
    std::print("trig_init>");
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    thrs1 = test_state::THR_CAN_INIT;
    thrs2 = test_state::THR_CAN_INIT;

    std::print("wait_send>");
    wait_both_for(thrs1, thrs2, test_state::THR_WAIT_FOR_CAN_SEND);
    std::print("trig_send>");
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    thrs1 = test_state::THR_CAN_SEND;
    thrs2 = test_state::THR_CAN_SEND;

    std::print("wait_reply>");
    wait_both_for(thrs1, thrs2, test_state::THR_WAIT_FOR_CAN_REPLY_BACK);
    std::print("trig_reply>");
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    thrs1 = test_state::THR_REPLY_BACK;
    thrs2 = test_state::THR_REPLY_BACK;

    std::print("wait_full_test>");
    wait_both_for(thrs1, thrs2, test_state::THR_WAIT_SPAM_EACH_OTHER);
    std::print("start_full_test>");
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    thrs1 = test_state::THR_DO_SPAM;
    thrs2 = test_state::THR_DO_SPAM;

    std::print("wait_final_check>");
    wait_both_for(thrs1, thrs2, test_state::THR_WAIT_FOR_FINISH, []{
        std::print("{:08d} : {:08d}\r", g_ref_threads[0], g_ref_threads[1]);
    });
    std::print("start_check>");
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    thrs1 = test_state::THR_FINISHING;
    thrs2 = test_state::THR_FINISHING;

    std::print("wait_end>");
    wait_both_for(thrs1, thrs2, test_state::THR_FINISHED);
    std::print("finish>");

    std::println("\n=== > PASSED! < ===");
}

void client(std::atomic<test_state>& sstate, bool do_send_first)
{
    const std::string broadcasting_msg = "Hi";
    std::shared_ptr<UDP_Host::UDP_Connection> cli;

    sstate = test_state::THR_WAIT_FOR_SIGNAL;
    wait_until(sstate, test_state::THR_CAN_INIT);

    UDP_Host host(sel_port, e_family::IPV4);
    host.enable_broadcast_ipv4(true);

    sstate = test_state::THR_WAIT_FOR_CAN_SEND;
    wait_until(sstate, test_state::THR_CAN_SEND);

    if (do_send_first) {
        UDP_Broadcaster cli(sel_port);
        if (!send_and_check_auto(cli, broadcasting_msg)) {
            throw std::runtime_error("Broadcaster could not send broadcast.");
        }

        auto drop = host.accept(); // it is expected for it to get a broadcast from itself.
        const auto cfg = drop->get_config();
        drop->close();

        std::println("@# Local drop conn debug (self): {}, {}, {}", cfg.address, cfg.port, static_cast<int>(cfg.family));
    }
    else {
        cli = host.accept();
        if (!cli) throw std::runtime_error("Accept resulted in nullptr.");
        const auto cfg = cli->get_config();

        std::println("@# Local cli conn debug: {}, {}, {}", cfg.address, cfg.port, static_cast<int>(cfg.family));

        if (!recv_udp_and_check_auto(*cli, broadcasting_msg)) {
            throw std::runtime_error("Broadcasted message did not match expected.");
        }
    }

    sstate = test_state::THR_WAIT_FOR_CAN_REPLY_BACK;
    wait_until(sstate, test_state::THR_REPLY_BACK);

    if (!do_send_first) {
        if (!send_and_check_auto(*cli, broadcasting_msg)) {
            throw std::runtime_error("Reply back did not work.");
        }
    }
    else {
        cli = host.accept();
        if (!cli) throw std::runtime_error("Accept resulted in nullptr.");
        const auto cfg = cli->get_config();

        std::println("@# Local cli 2 conn debug: {}, {}, {}", cfg.address, cfg.port, static_cast<int>(cfg.family));

        if (!recv_udp_and_check_auto(*cli, broadcasting_msg)) {
            throw std::runtime_error("Broadcasted message did not match expected.");
        }
    }

    sstate = test_state::THR_WAIT_SPAM_EACH_OTHER;
    wait_until(sstate, test_state::THR_DO_SPAM);

    constexpr size_t min_package_len = 20;
    constexpr size_t max_package_len = 200;
    auto& counter = g_ref_threads[do_send_first ? 1 : 0];
    char BUFFER[max_package_len];

    struct diag {
        size_t sum_sent = 0;
        size_t sum_recv = 0;
    } curr;

    counter = 0;
    for(size_t p = 0; p < sel_test_spam_msgs_size; ++p) {
        const size_t msg_len = generate_random_in_range(min_package_len, max_package_len);
        const auto msg = generate_random_string(msg_len);

        if (!send_and_check_auto(*cli, msg)) {
            throw std::runtime_error("Error on sending messages. Failed at ^" + std::to_string(curr.sum_sent) + "/" + std::to_string(curr.sum_recv) + "v byte(s).");
        }
        curr.sum_sent += msg_len;

        const auto recvd = cli->recv(BUFFER, max_package_len);
        if (recvd <= 0) {
            throw std::runtime_error("Error on receiving messages. Failed at ^" + std::to_string(curr.sum_sent) + "/" + std::to_string(curr.sum_recv) + "v byte(s).");            
        }

        curr.sum_recv += recvd;
        ++counter;
    }

    sstate = test_state::THR_WAIT_FOR_FINISH;
    wait_until(sstate, test_state::THR_FINISHING);

    if (cli->send((char*)&curr, sizeof(diag)) != sizeof(diag)) {
        throw std::runtime_error("Error on sending final package for comparison. Failed at ^" + std::to_string(curr.sum_sent) + "/" + std::to_string(curr.sum_recv) + "v byte(s).");
    }

    diag other_side;
    if (cli->recv((char*)&other_side, sizeof(diag)) != sizeof(diag)) {
        throw std::runtime_error("Error on receiving final package for comparison. Failed at ^" + std::to_string(curr.sum_sent) + "/" + std::to_string(curr.sum_recv) + "v byte(s).");
    }

    if (curr.sum_recv != other_side.sum_sent) {
        throw std::runtime_error("Mismatch on package sizes. Recv here = '" + std::to_string(curr.sum_recv) + "' != sent there = '" + std::to_string(other_side.sum_sent) + "'.");
    }
    if (curr.sum_sent != other_side.sum_recv) {
        throw std::runtime_error("Mismatch on package sizes. Recv there = '" + std::to_string(other_side.sum_recv) + "' != sent here = '" + std::to_string(curr.sum_sent) + "'.");
    }

    sstate = test_state::THR_FINISHED;
}