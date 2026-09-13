#ifndef SERVER_H
#define SERVER_H
#include <string>
#include <Network/Socket.h>
#include "KeyValueStore/KeyValueStore.h"
#include "Protocol/Command.h"

class Server {
    private:
    int serverSocket = -1;

    //KVS functions
    KeyValueStore store;
    std::string executeCommand(const Command& command);

    //Helper Functions
    bool receiveFrame(int clientSocket, std::string& message);
    void sendFrame(int clientSocket, const std::string& frame);
    void sendAll(int clientSocket, const char* data, std::size_t size);

    public:
    Server();
    Server(const Server&) = delete;
    Server& operator=(const Server&) = delete;
    ~Server();
    int acceptSocket();
    void handleClient(Socket& socket);

};


#endif