#include <thread>
#include <sys/socket.h>
#include <iostream>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <filesystem>
#include <unistd.h>
#include "Server/Server.h"

constexpr std::size_t MAX_FRAME_SIZE = 1024 * 1024 * 20;

std::string receiveFrame(int fd) {
    uint32_t messageSize;
    std::size_t totalReceivedSize = 0;
    while (totalReceivedSize < sizeof(uint32_t)) {
        ssize_t receivedSize = recv(fd, reinterpret_cast<char*>(&messageSize) + totalReceivedSize,
                                    sizeof(uint32_t) - totalReceivedSize, 0);
        if (receivedSize == -1) {
            throw std::runtime_error("Failed to receive data from client");
        }
        if (receivedSize == 0) {
            throw std::runtime_error("Failed to receive data from client");
        }
        totalReceivedSize += static_cast<std::size_t>(receivedSize);
    }
    messageSize = ntohl(messageSize);
    if (messageSize > MAX_FRAME_SIZE) {
        throw std::runtime_error("Message too large");
    }
    std::string message(messageSize, '\0');
    totalReceivedSize = 0;
    while (totalReceivedSize < (messageSize)) {
        ssize_t receivedSize = recv(fd, message.data() + totalReceivedSize, messageSize - totalReceivedSize,
                                    0);
        if (receivedSize == -1) {
            throw std::runtime_error("Failed to receive data from client");
        }
        if (receivedSize == 0) {
            throw std::runtime_error("client disconnected");
        }
        totalReceivedSize += static_cast<std::size_t>(receivedSize);
    }
    return message;
}

std::string sendFrame(int fd, const std::string& request) {
    uint32_t requestSize = request.size();
    uint32_t networkSize = htonl(requestSize);

    const char* dataSize = reinterpret_cast<const char*>(&networkSize);
    for (std::size_t i = 0; i < sizeof(networkSize); ++i) {
        ssize_t sendSize = send(fd, dataSize + i, 1, 0);
        if (sendSize == -1) {
            throw std::runtime_error("Failed to send data to client");
        }
        if (sendSize == 0) {
            throw std::runtime_error("Failed to send data to client");
        }
    }
    for (std::size_t i = 0; i < requestSize; i++) {
        ssize_t sendSize = send(fd, request.data() + i, 1, 0);
        if (sendSize == -1) {
            throw std::runtime_error("Failed to send data to client");
        }
        if (sendSize == 0) {
            throw std::runtime_error("Failed to send data to client");
        }
    }
    return receiveFrame(fd);
}


int connectToServer(const std::string& request) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) {
        throw std::runtime_error("Failed to create socket");
    }
    sockaddr_in serveraddr_in{};
    serveraddr_in.sin_family = AF_INET;
    serveraddr_in.sin_port = htons(8080);
    if (inet_pton(AF_INET, "127.0.0.1", &serveraddr_in.sin_addr) <= 0) {
        close(fd);
        throw std::runtime_error("Invalid server address");
    }
    if (connect(fd, reinterpret_cast<sockaddr*>(&serveraddr_in), sizeof(serveraddr_in)) == -1) {
        close(fd);
        throw std::runtime_error("Failed to connect to server");
    }
    try {
        std::cout << sendFrame(fd, request) << std::endl;
        return fd;
    }
    catch (...) {
        close(fd);
        throw;
    }
}


int main() {
    std::filesystem::remove("shutdown_test.db");
    Server server("shutdown_test.db");

    std::thread serverThread([&server]() {
        server.run();
    });

    Socket clientA(connectToServer("set test server"));
    Socket clientB(connectToServer("get test"));

    server.stop();

    serverThread.join();

    std::cout << "Shutdown test passed" << std::endl;
    return 0;
}