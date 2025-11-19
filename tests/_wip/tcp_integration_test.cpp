#include <Lunaris/socket.h>

#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>

namespace LS = Lunaris::Socket;

void main_host(bool& keep_alive, std::condition_variable& cond)
{
    std::cout << "Begin of host\n";

    LS::Server* server = new LS::TCP::Server(3369);

    keep_alive = true;
    cond.notify_all();

    while(keep_alive) {
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

    std::cout << "End of host\n";
    delete server;
}

int main() {
    std::cout << "Starting test...\n";

    bool keep_alive = false;
    std::condition_variable cond;

    std::thread thr(main_host, keep_alive, cond);

    {
        std::mutex m;
        std::unique_lock<std::mutex> l(m);
        while(!keep_alive) cond.wait_for(l, std::chrono::milliseconds(100));
    }


    std::cout << "Wait of client\n";

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    std::cout << "Begin of client\n";

    LS::Client* cli = new LS::TCP::Client("127.0.0.1", 3369);

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

    std::cout << "End of client\n";
    delete cli;
    keep_alive = false;
    thr.join();
    return 0;
}