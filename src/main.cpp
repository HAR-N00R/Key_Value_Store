#include <iostream>
#include <thread>
#include "Server/Server.h"

int main() {
    std::cout << std::string(60, '=') << std::endl;
    std::cout << "Welcome to Key Value Store" << std::endl;
    std::cout << std::string(60, '=') << std::endl;

    Server server("data.db");
    std::thread serverThread([&server]()
    {
        server.run();
    });
    int stopTest;
    std::cin >> stopTest;
    if (stopTest == 1) {
        std::cout << std::string(60, '=') << std::endl;
        server.stop();
        std::cout << std::string(60, '-') << std::endl;
    }
    serverThread.join();

    return 0;
}