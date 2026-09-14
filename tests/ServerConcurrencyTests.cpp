#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdexcept>
#include <iostream>
#include <string>
#include <thread>
#include <sstream>
#include "Network/Socket.h"
#include "Server/Server.h"
#include "KeyValueStore/KeyValueStore.h"
#include <filesystem>
#include <atomic>
#include <vector>

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
        std::string expected = std::to_string(i) + ": " + std::to_string(i);
        if (expected != response) {
            return false;
        }
    }
    return true;
}

void valuePersistentTest(const std::string& key, const std::string& value) {
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
    for (int i = 0; i < 100; i++) {
        std::string request = "set " + key + " " + value;
        sendFrame(fd, request);
    }
}

std::string checkValuePersistence(const std::string& key) {
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
    std::string request = "get " + key;
    std::string response = sendFrame(fd, request);
    if (response == "Key doesn't exist") {
        return response;
    }
    std::stringstream ss(response);
    std::string receivedKey;
    std::string receivedValue;
    std::getline(ss, receivedKey, ':');
    std::getline(ss, receivedValue);
    receivedValue.erase(0, 1);
    return receivedValue;
}

void compactRequest() {
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
    std::string request = "compact";
    sendFrame(fd, request);
}

bool compactionValidation(KeyValueStore& store) {
    for (int i = 0; i < 800; i++) {
        std::string key = std::to_string(i);
        std::string response = store.getValue(key);
        if (response == "Key doesn't exist") {
            return false;
        }
        if (key != response) {
            return false;
        }
    }
    return true;
}

int main() {
    std::filesystem::remove("test_data.db");
    std::string message;
    std::string valueBeforeRestart;
    std::string valueAfterRestart;
    std::atomic_bool serverIsRunning(false);
    {
        std::thread serverThread([&serverIsRunning]()
        {
            Server server("test_data.db");
            serverIsRunning.store(true);
            serverIsRunning.notify_one();
            std::vector<std::thread> clientThreads;
            for (int i = 0; i < 15; i++) {
                int fd = server.acceptSocket();
                clientThreads.emplace_back([&server, fd]()
                {
                    Socket client(fd);
                    server.handleClient(client);
                });
            }
            for (auto& thread : clientThreads) {
                thread.join();
            }
        });
        serverIsRunning.wait(false);
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
            message += "Data validity test failed\n";
        }
        std::thread clientThread9(valuePersistentTest, "testKey", "1234");
        std::thread clientThread10(valuePersistentTest, "testKey", "4321");
        std::thread clientThread11(valuePersistentTest, "testKey", "5678");
        std::thread clientThread12(valuePersistentTest, "testKey", "8765");
        clientThread9.join();
        clientThread10.join();
        clientThread11.join();
        clientThread12.join();
        valueBeforeRestart = checkValuePersistence("testKey");
        compactRequest();
        serverThread.join();
    }

    KeyValueStore store("test_data.db");
    valueAfterRestart = store.getValue("testKey");
    if (valueBeforeRestart != valueAfterRestart) {
        message += "Value persistence test failed";
    }
    if (!compactionValidation(store) || valueBeforeRestart != valueAfterRestart) {
        message += "Data compaction test failed\n";
    }
    if (message.empty()) {
        std::cout << "All Server concurrency tests passed" << std::endl;
        return 0;
    }
    else {
        std::cout << message << std::endl;
        return 1;
    }
}
