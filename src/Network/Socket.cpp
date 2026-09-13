#include "Socket.h"
#include <stdexcept>
#include <unistd.h>


Socket::Socket(int soc) : socketFd(soc) {
    if (socketFd < 0) {
        throw std::runtime_error("Invalid socket");
    }
}

int Socket::getSocket() const {
    return socketFd;
}

Socket::~Socket() {
    if (socketFd >= 0) {
        close(socketFd);
    }
}