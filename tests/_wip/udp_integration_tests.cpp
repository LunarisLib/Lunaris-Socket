#include <Lunaris/socket.h>

#include <iostream>
#include <thread>

namespace LS = Lunaris::Socket;

void test_direct();
void test_broadcast();

int main() {
    std::cout << "Starting tests...\n";

    test_direct();
    test_broadcast();
}

void test_direct() {
    LS::Server* server = new LS::UDP::Server(3370);
    LS::Client* client = new LS::UDP::Client("127.0.0.1", 3370);
    bool keep_alive = true;

    std::thread thr([&]() {
        while (keep_alive) {
            LS::Client* cli = server->listen(5000); // ms

            if (!cli) {
                std::cout << "Waiting for clients...\n";
                continue;
            }

            LS::DataFrame frame;
            if (cli->recv(frame, 5000)) {
                std::cout << "Got frame: " << frame.data() << "\n";
                cli->send(frame);
            }
            else {
                std::cout << "Did not get frame. Skip.\n";
            }

            delete cli;
        }
    });
    
    LS::DataFrame frame;
    frame << "This is a message from the client! Hello, friend!";

    cli->send(frame);
    LS::DataFrame frame_2;
    if (cli->recv(frame_2, 5000)) {
        std::cout << "Client got back frame: " << frame_2.data() << "\n";
    }
    else {
        std::cout << "Client failed to receive back.\n";
    }

}

void test_broadcast() {

}