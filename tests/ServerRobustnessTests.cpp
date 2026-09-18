#include <cassert>
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
    std::size_t totalSentSize = 0;
    while (totalSentSize < sizeof(uint32_t)) {
        ssize_t sendSize = send(fd, dataSize + totalSentSize, sizeof(uint32_t) - totalSentSize, 0);
        if (sendSize == -1) {
            throw std::runtime_error("Failed to send data to client");
        }
        if (sendSize == 0) {
            throw std::runtime_error("Failed to send data to client");
        }
        totalSentSize += static_cast<std::size_t>(sendSize);
    }
    totalSentSize = 0;
    while (totalSentSize < requestSize) {
        ssize_t sendSize = send(fd, request.data() + totalSentSize, requestSize - totalSentSize , 0);
        if (sendSize == -1) {
            throw std::runtime_error("Failed to send data to client");
        }
        if (sendSize == 0) {
            throw std::runtime_error("Failed to send data to client");
        }
        totalSentSize += static_cast<std::size_t>(sendSize);
    }
    return receiveFrame(fd);
}


void sendOverSizedFrames(int fd) {
    uint32_t requestSize = MAX_FRAME_SIZE * 2;
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
    return;
}

void sendTruncatedFrames(int fd, const std::string& request) {
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
    for (std::size_t i = 0; i < requestSize / 2; i++) {
        ssize_t sendSize = send(fd, request.data() + i, 1, 0);
        if (sendSize == -1) {
            throw std::runtime_error("Failed to send data to client");
        }
        if (sendSize == 0) {
            throw std::runtime_error("Failed to send data to client");
        }
    }
    return;
}

void sendTruncatedHeader(int fd, const std::string& request) {
    uint32_t requestSize = request.size();
    uint32_t networkSize = htonl(requestSize);

    const char* dataSize = reinterpret_cast<const char*>(&networkSize);
    for (std::size_t i = 0; i < sizeof(networkSize) - 2; ++i) {
        ssize_t sendSize = send(fd, dataSize + i, 1, 0);
        if (sendSize == -1) {
            throw std::runtime_error("Failed to send data to client");
        }
        if (sendSize == 0) {
            throw std::runtime_error("Failed to send data to client");
        }
    }
    return;
}

void sendDisconnect(int fd, const std::string& request) {
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
    return;
}

void connectToServer(const std::string& request, const int method) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) {
        throw std::runtime_error("failed to create socket");
    }
    if (method == 4) {
        struct linger option{};
        option.l_onoff = 1;
        option.l_linger = 0;
        if (setsockopt(fd, SOL_SOCKET, SO_LINGER, &option, sizeof(option)) == -1) {
            close(fd);
            throw std::runtime_error("Failed to set SO_LINGER");
        }
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
        if (method == 1) {
            sendTruncatedFrames(fd, request);
        }
        else if (method == 2) {
            sendTruncatedHeader(fd, request);
        }
        else if (method == 3) {
            sendOverSizedFrames(fd);
        }
        else if (method == 4) {
            sendDisconnect(fd, request);
        }
        close(fd);
    }
    catch (...) {
        close(fd);
        throw;
    }
}

std::string connectToServer(const std::string& request) {
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
        std::string response = sendFrame(fd, request);
        close(fd);
        return response;
    }
    catch (...) {
        close(fd);
        throw;
    }
}

int main() {
    std::filesystem::remove("server_robustness_test.db");
    Server server("server_robustness_test.db");

    std::thread serverThread([&server]()
    {
        server.run();
    });

    std::string largeValue(10*1024*1024,'A');
    assert(connectToServer("set bulky " + largeValue) == "Key Set");
    connectToServer("get bulky", 4);
    assert(connectToServer("set Hello World!") == "Key Set");
    connectToServer("set test server", 1);
    assert(connectToServer("get test") == "Key doesn't exist");
    connectToServer("set beta alpha", 2);
    assert(connectToServer("get beta") == "Key doesn't exist");
    connectToServer("set charlie delta", 3);
    assert(connectToServer("get charlie") == "Key doesn't exist");
    assert(connectToServer("get Hello")== "Hello: World!");

    server.stop();
    serverThread.join();

    std::cout << "Robustness test passed" << std::endl;
    return 0;
}
