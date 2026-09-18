#include <iostream>
#include <thread>
#include "Server/Server.h"

int main() {
    try {
        Server server("data.db");
        std::thread serverThread([&server]()
        {
            server.run();
        });

        std::cout << std::string(60, '=') << std::endl;
        std::cout << "Server running on port 8080." << std::endl;
        std::cout << std::string(60, '=') << std::endl;
        std::cout << "Press enter to shutdown server";
        std::string line;
        std::getline(std::cin, line);

        std::cout << std::string(60, '=') << std::endl;
        server.stop();
        std::cout << "Server shutdown successful" << std::endl;
        std::cout << std::string(60, '=') << std::endl;

        serverThread.join();
    }
    catch (const std::exception& e) {
        std::cerr << "Server error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}