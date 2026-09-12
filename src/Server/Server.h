#ifndef SERVER_H
#define SERVER_H
#include <string>
#include <KeyValueStore/KeyValueStore.h>
#include "Protocol/Command.h"

class Server {
    private:
    int serverSocket = -1;
    int acceptSocket();



    //KVS functions

    KeyValueStore store;
    std::string executeCommand(const Command& command);

    //Helper Functions
    std::string receiveFrame(int clientSocket);
    void sendFrame(int clientSocket, const std::string& frame);
    void sendAll(int clientSocket, const char* data, std::size_t size);

    public:
    Server();
    Server(const Server&) = delete;
    Server& operator=(const Server&) = delete;
    ~Server();
    void handleClient();

};


#endif