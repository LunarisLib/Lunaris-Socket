#include <Lunaris/socket.h>

#include <iostream>
#include <thread>
#include <atomic>
#include <future>
#include <functional>
#include <string>
#include <memory>

namespace LS = Lunaris::Socket;
using server_ptr = std::shared_ptr<LS::Server>;
using client_ptr = std::shared_ptr<LS::Client>;

constexpr size_t TIMEOUT = 1000;


int client_send_recv(client_ptr cli);
int host_recv_send_until(server_ptr server, std::atomic<bool>& keep_alive);

int test(const std::string& name, std::function<server_ptr()> get_server, std::function<client_ptr()> get_client);


int main() {
    int res;
    
    res = test("[TCP v4]", []{return server_ptr(new LS::TCP::Server(3369));}, []{return client_ptr(new LS::TCP::Client("127.0.0.1", 3369));});
    if (res != 0) {
        std::cout << "Failed at test TCP v4!";
        return 1;
    }

    //test("[UDP v4]", []{return server_ptr(new LS::UDP::Server(3369));}, []{return client_ptr(new LS::UDP::Client("127.0.0.1", 3369));});

    return 0;
}


int test(const std::string& name, std::function<server_ptr()> get_server, std::function<client_ptr()> get_client)
{
    auto server = get_server(); //new LS::TCP::Server(3369);
    std::atomic<bool> keep_alive = false;

    std::future<int> fut = std::async(std::launch::async, [&]{ return host_recv_send_until(server, keep_alive); });
   
    const auto end_server = [&]() -> int {
        keep_alive = false;
        const int res = fut.get();
        if (res != 0) {
            std::cout << name << " Error on server side: code: " << res << "\n";
        }
        return res;
    };

    while(!keep_alive.load())
        std::this_thread::sleep_for(std::chrono::milliseconds(50));

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    auto cli = get_client(); //new LS::TCP::Client("127.0.0.1", 3369);

    if (int res = client_send_recv(cli); res != 0) {
        std::cout << name << " Error on client side: code: " << res << "\n";
        end_server();
        return res;
    }

    return end_server();
}

int client_send_recv(client_ptr cli)
{
    LS::DataFrame frame;
    frame << "This is a message from the client! Hello, friend!";

    cli->send(frame);
    LS::DataFrame frame_2;
    if (cli->recv(frame_2, TIMEOUT)) {
        std::cout << "Client: Got back frame: " << frame_2.data() << "\n";
    }
    else {
        std::cout << "Client: Failed to receive back.\n";
        return 1;
    }

    return 0;
}

int host_recv_send_until(server_ptr server, std::atomic<bool>& keep_alive)
{
    std::cout << "Server: Begin of host\n";
    keep_alive = true;

    while(keep_alive.load()) {
        LS::Client* cli = server->listen(TIMEOUT); // ms

        if (!cli) {
            if (!keep_alive) return 0;
            std::cout << "Server: Timed out.\n";
            return 1;
        }

        LS::DataFrame frame;
        if (cli->recv(frame, TIMEOUT)) {
            std::cout << "Server: Got frame: " << frame.data() << "\n";
            cli->send(frame);
        }
        else {
            std::cout << "Server: Did not get frame!\n";
            delete cli;
            std::cout << "End of host\n";
            return 1;
        }

        delete cli;
    }

    return 0;
}

