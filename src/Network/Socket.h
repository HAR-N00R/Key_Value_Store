#ifndef SOCKET_H
#define SOCKET_H


class Socket {
private:
    int socketFd = -1;

public:
    explicit Socket(int soc);
    Socket(const Socket&) = delete;
    Socket& operator=(const Socket&) = delete;
    ~Socket();
    int getSocket() const;

};


#endif