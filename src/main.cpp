#include <iostream>
#include <thread>
#include "Server/Server.h"

int main() {
    std::cout << std::string(60, '=') << std::endl;
    std::cout << "Welcome to Key Value Store" << std::endl;
    std::cout << std::string(60, '=') << std::endl;

    Server server("data.db");
    server.run();

    return 0;
}