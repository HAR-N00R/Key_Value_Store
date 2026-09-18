#ifndef SERVER_H
#define SERVER_H
#include <string>
#include <Network/Socket.h>
#include <atomic>
#include "KeyValueStore/KeyValueStore.h"
#include "Protocol/Command.h"
#include <mutex>
#include <vector>

class Server {
    private:
    int serverSocket = -1;
    std::atomic<bool> isRunning{true};

    std::vector<int> activeClients;
    std::mutex clientsMutex;

    //KVS functions
    KeyValueStore store;
    std::string executeCommand(const Command& command);

    //Helper Functions
    bool receiveFrame(int clientSocket, std::string& message);
    void sendFrame(int clientSocket, const std::string& frame);
    void sendAll(int clientSocket, const char* data, std::size_t size);

    public:
    Server(const std::string& filePath);
    Server(const Server&) = delete;
    Server& operator=(const Server&) = delete;
    ~Server();
    int acceptSocket();
    void handleClient(Socket& socket);

    void run();
    void stop();

};


#endif