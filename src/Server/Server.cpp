#include "Server.h"
#include <iostream>
#include <Protocol/CommandParser.h>
#include <stdexcept>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

// Key = 1MB
// Value = 16MB
constexpr std::size_t MAX_FRAME_SIZE = 1024 * 1024 * 20;


Server::Server(const std::string& filePath) : store(filePath) {
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        throw std::runtime_error("Failed to open socket");
    }
    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(8080);

    int opt = 1;
    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        close(serverSocket);
        throw std::runtime_error("Failed to set SO_REUSEADDR");
    }

    if (bind(serverSocket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) == -1) {
        close(serverSocket);
        throw std::runtime_error("Failed to bind socket");
    }
    if (listen(serverSocket, SOMAXCONN) == -1) {
        close(serverSocket);
        throw std::runtime_error("Failed to listen on socket");
    }
}

Server::~Server() {
    if (serverSocket >= 0) {
        close(serverSocket);
    }
}

int Server::acceptSocket() {
    sockaddr_in clientAddr{};
    socklen_t len = sizeof(clientAddr);
    int clientSocket = accept(serverSocket, reinterpret_cast<sockaddr*>(&clientAddr), &len);
    if (clientSocket == -1) {
        throw std::runtime_error("Failed to accept connection");
    }
    return clientSocket;
}

void Server::handleClient(Socket& client) {
    int clientSocket = client.getSocket();
    bool connected = true;
    try {
        CommandParser parser;
        while (connected) {
            std::string request;
            connected = receiveFrame(clientSocket, request);
            if (!connected) {
                break;
            }
            std::string response = executeCommand(parser.parse(request));
            sendFrame(clientSocket, response);
        }
    }
    catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
    }
}

bool Server::receiveFrame(int clientSocket, std::string& message) {
    uint32_t messageSize = 0;
    std::size_t totalReceivedSize = 0;
    while (totalReceivedSize < sizeof(uint32_t)) {
        ssize_t receivedSize = recv(clientSocket, reinterpret_cast<char*>(&messageSize) + totalReceivedSize,
            (sizeof(uint32_t)- totalReceivedSize), 0);
        if (receivedSize == -1) {
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
            throw std::runtime_error("Failed to send data to client");
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