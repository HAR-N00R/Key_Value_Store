#include "Server.h"
#include "Protocol/CommandParser.h"
#include <iostream>
#include <vector>
#include <stdexcept>
#include <thread>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cerrno>

// command + 16 MB value + protocol overhead
constexpr std::size_t MAX_FRAME_SIZE = 1024 * 1024 * 20;


Server::Server(const std::string& filePath) : store(filePath) {
    serverSocket.store(socket(AF_INET, SOCK_STREAM, 0));
    if (serverSocket.load() < 0) {
        throw std::runtime_error("Failed to open socket");
    }
    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(8080);

    int opt = 1;
    if (setsockopt(serverSocket.load(), SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        close(serverSocket.load());
        throw std::runtime_error("Failed to set SO_REUSEADDR");
    }

    if (bind(serverSocket.load(), reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) == -1) {
        close(serverSocket.load());
        throw std::runtime_error("Failed to bind socket");
    }
    if (listen(serverSocket.load(), SOMAXCONN) == -1) {
        close(serverSocket.load());
        throw std::runtime_error("Failed to listen on socket");
    }
}

Server::~Server() {
    if (serverSocket.load() >= 0) {
        close(serverSocket.load());
    }
}

void Server::run() {
    std::vector<std::thread> threads;
    while (isRunning.load()) {
        int fd = acceptSocket();
        std::unique_lock lock(clientsMutex);
        if (!isRunning.load()) {
            if (fd >= 0) {
                close(fd);
            }
            break;
        }
        activeClients.push_back(fd);
        threads.emplace_back([fd, this]()
        {
            Socket client(fd);
            try {
                handleClient(client);
            }
            catch (const std::exception& e) {
                std::cerr << e.what() << std::endl;
            }
            catch (...) {
                std::cerr << "Unknown error occurred" << std::endl;
            }
            std::unique_lock lock(clientsMutex);
            std::erase(activeClients, fd);
        });
    }
    for (auto& thread : threads) {
        thread.join();
    }
}

void Server::stop() {
    isRunning.store(false);
    int fd = serverSocket.exchange(-1);
    if (fd >= 0) {
        shutdown(fd, SHUT_RDWR);
        close(fd);
    }
    std::unique_lock lock(clientsMutex);
    for (int fd : activeClients) {
        shutdown(fd, SHUT_RDWR);
    }
}

int Server::acceptSocket() {
    sockaddr_in clientAddr{};
    socklen_t len = sizeof(clientAddr);
    while (true) {
        if (serverSocket.load() < 0 || !isRunning.load()) {
            return -1;
        }
        int clientSocket = accept(serverSocket.load(), reinterpret_cast<sockaddr*>(&clientAddr), &len);
        if (clientSocket == -1) {
            if (errno == EINTR) {
                continue;
            }
            if (!isRunning.load()) {
                return -1;
            }
            throw std::runtime_error("Failed to accept connection");
        }
        int opt = 1;
        if (setsockopt(clientSocket, SOL_SOCKET, SO_NOSIGPIPE, &opt, sizeof (opt)) == -1) {
            close(clientSocket);
            throw std::runtime_error("Failed to set NO-SIGPIPE");
        }
        if (!isRunning.load()) {
            close(clientSocket);
            return -1;
        }
        return clientSocket;
    }
}

void Server::handleClient(Socket& client) {
    int clientSocket = client.getSocket();

    CommandParser parser;
    while (true) {
        std::string request;
        if (!receiveFrame(clientSocket, request)) {
            break;
        }
        std::string response = executeCommand(parser.parse(request));
        sendFrame(clientSocket, response);
    }
}

bool Server::receiveFrame(int clientSocket, std::string& message) {
    uint32_t messageSize = 0;
    std::size_t totalReceivedSize = 0;
    while (totalReceivedSize < sizeof(uint32_t)) {
        ssize_t receivedSize = recv(clientSocket, reinterpret_cast<char*>(&messageSize) + totalReceivedSize,
                                    (sizeof(uint32_t) - totalReceivedSize), 0);
        if (receivedSize == -1) {
            if (errno == EINTR) {
                continue;
            }
            throw std::runtime_error("Failed to receive data from client");
        }
        if (receivedSize == 0 && totalReceivedSize == 0) {
            return false;
        }
        if (receivedSize == 0 && totalReceivedSize != 0) {
            throw std::runtime_error("Client disconnected while receiving");
        }
        totalReceivedSize += static_cast<std::size_t>(receivedSize);
    }
    messageSize = ntohl(messageSize);
    if (messageSize > MAX_FRAME_SIZE) {
        throw std::runtime_error("Message too large");
    }
    message.resize(messageSize, '\0');
    totalReceivedSize = 0;
    while (totalReceivedSize < (messageSize)) {
        ssize_t receivedSize = recv(clientSocket, message.data() + totalReceivedSize, messageSize - totalReceivedSize,
                                    0);
        if (receivedSize == -1) {
            if (errno == EINTR) {
                continue;
            }
            throw std::runtime_error("Failed to receive data from client");
        }
        if (receivedSize == 0) {
            throw std::runtime_error("Client disconnected while receiving");
        }
        totalReceivedSize += static_cast<std::size_t>(receivedSize);
    }
    return true;
}

void Server::sendFrame(int clientSocket, const std::string& frame) {
    if (frame.size() > MAX_FRAME_SIZE) {
        throw std::runtime_error("Message too large to send");
    }
    uint32_t frameSize = static_cast<uint32_t>(frame.size());
    frameSize = htonl(frameSize);
    sendAll(clientSocket, reinterpret_cast<const char*>(&frameSize), sizeof(uint32_t));
    sendAll(clientSocket, frame.data(), frame.size());
}

void Server::sendAll(int clientSocket, const char* data, std::size_t size) {
    std::size_t totalSentSize = 0;
    while (totalSentSize < size) {
        ssize_t sendSize = send(clientSocket, data + totalSentSize, size - totalSentSize, 0);
        if (sendSize == -1) {
            if (errno == EINTR) {
                continue;
            }
            if (errno == EPIPE) {
                throw std::runtime_error("Client disconnected while sending");
            }
            else {
                throw std::runtime_error("Failed to send data to client");
            }
        }
        if (sendSize == 0) {
            throw std::runtime_error("Failed to send data to client");
        }
        totalSentSize += static_cast<std::size_t>(sendSize);
    }
}

std::string Server::executeCommand(const Command& command) {
    if (command.type == CommandType::Unknown) {
        return "Unknown Command";
    }
    try {
        if (command.type == CommandType::Compact && command.key.empty() && command.value.empty()) {
            store.compact();
            return "Compaction Complete";
        }
        if (command.key.empty()) {
            return "Invalid Key";
        }

        if (command.type == CommandType::Set) {
            store.setValue(command.key, command.value);
            return "Key Set";
        }
        else if (command.type == CommandType::Get && command.value.empty()) {
            try {
                std::string value = command.key + ": " + store.getValue(command.key);
                return value;
            }
            catch (const std::out_of_range& e) {
                return "Key doesn't exist";
            }
        }
        else if (command.type == CommandType::Exists && command.value.empty()) {
            if (store.exists(command.key)) {
                return "Key exists";
            }
            else {
                return "Key doesn't exist";
            }
        }
        else if (command.type == CommandType::Delete && command.value.empty()) {
            if (!store.removeKey(command.key)) {
                return "Key doesn't exist";
            }
            return "Key deleted";
        }
        else {
            return "Invalid Command";
        }
    }
    catch (const std::runtime_error& error) {
        return error.what();
    }
}
