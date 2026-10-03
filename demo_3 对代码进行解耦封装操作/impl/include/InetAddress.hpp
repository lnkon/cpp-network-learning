#pragma once
#include <arpa/inet.h>
#include <string>

class InetAddress{
private:
    struct sockaddr_in addr;
    socklen_t addr_len;
public:
    InetAddress();
    InetAddress(const std::string&, uint16_t port);
    ~InetAddress();

    void setInetAddr(sockaddr_in, socklen_t);
    sockaddr_in getAddr();
    socklen_t getAddr_len();
};