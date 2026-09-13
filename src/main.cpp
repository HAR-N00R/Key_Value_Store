#include <iostream>
#include <thread>
#include "Server/Server.h"

int main() {
    std::cout << std::string(60, '=') << std::endl;
    std::cout << "Welcome to Key Value Store" << std::endl;
    std::cout << std::string(60, '=') << std::endl;

    Server server;
    while (true) {
        int fd = server.acceptSocket();
        std::thread t([&server, fd](){
            Socket client(fd);
            server.handleClient(client);
        });
        t.detach();
    }

    return 0;
}