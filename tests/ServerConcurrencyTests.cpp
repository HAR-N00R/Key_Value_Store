#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdexcept>
#include <iostream>
#include <string>
#include <thread>
#include <sstream>
#include "Network/Socket.h"

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
    std::size_t totalSent = 0;
    for (std::size_t i = 0; i < requestSize; i++) {
        ssize_t sendSize = send(fd, request.data() + i, 1, 0);
        if (sendSize == -1) {
            throw std::runtime_error("Failed to send data to client");
        }
        if (sendSize == 0) {
            throw std::runtime_error("Failed to send data to client");
        }
        totalSent += static_cast<std::size_t>(sendSize);
    }

    return receiveFrame(fd);
}

void concurrencyTest(int num) {
    Socket client(socket(AF_INET, SOCK_STREAM, 0));
    int fd = client.getSocket();
    sockaddr_in serveraddr_in{};
    serveraddr_in.sin_family = AF_INET;
    serveraddr_in.sin_port = htons(8080);
    if (inet_pton(AF_INET, "127.0.0.1", &serveraddr_in.sin_addr) <= 0) {
        throw std::runtime_error("Invalid server address");
    }
    if (connect(fd, reinterpret_cast<sockaddr*>(&serveraddr_in), sizeof(serveraddr_in)) == -1) {
        throw std::runtime_error("Failed to connect to server");
    }
    std::cout << "Connection established" << std::endl;
    for (int i = num; i < num + 100; i++) {
        std::string request = "set " + std::to_string(i) + " " + std::to_string(i);
        sendFrame(fd, request);
    }
}

bool dataValidityTest() {
    Socket client(socket(AF_INET, SOCK_STREAM, 0));
    int fd = client.getSocket();
    sockaddr_in serveraddr_in{};
    serveraddr_in.sin_family = AF_INET;
    serveraddr_in.sin_port = htons(8080);
    if (inet_pton(AF_INET, "127.0.0.1", &serveraddr_in.sin_addr) <= 0) {
        throw std::runtime_error("Invalid server address");
    }
    if (connect(fd, reinterpret_cast<sockaddr*>(&serveraddr_in), sizeof(serveraddr_in)) == -1) {
        throw std::runtime_error("Failed to connect to server");
    }
    for (int i = 0; i < 800; i++) {
        std::string request = "get " + std::to_string(i);
        std::string response = sendFrame(fd, request);
        if (response == "Key doesn't exist") {
            return false;
        }
        std::stringstream ss(response);
        std::string key;
        std::string value;
        std::getline(ss, key, ':');
        std::getline(ss, value);
        value.erase(0, 1);
        if (key != value) {
            return false;
        }
    }
    return true;
}

int main() {
    std::thread clientThread1(concurrencyTest, 0);
    std::thread clientThread2(concurrencyTest, 100);
    std::thread clientThread3(concurrencyTest, 200);
    std::thread clientThread4(concurrencyTest, 300);
    std::thread clientThread5(concurrencyTest, 400);
    std::thread clientThread6(concurrencyTest, 500);
    std::thread clientThread7(concurrencyTest, 600);
    std::thread clientThread8(concurrencyTest, 700);
    clientThread1.join();
    clientThread2.join();
    clientThread3.join();
    clientThread4.join();
    clientThread5.join();
    clientThread6.join();
    clientThread7.join();
    clientThread8.join();
    if (!dataValidityTest()) {
        std::cout << "Data validity test failed" << std::endl;
        return 1;
    }
    std::cout << "Data validity test passed" << std::endl;
    return 0;
}
