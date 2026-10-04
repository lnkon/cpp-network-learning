#include "../include/Acceptor.hpp"
#include "../include/Socket.hpp"
#include "../include/InetAddress.hpp"
#include "../include/Channel.hpp"
#include <iostream>

Acceptor::Acceptor(EventLoop *_loop):loop(_loop), sock(nullptr), acceptChannel(nullptr){
    sock = new Socket();
    InetAddress *addr = new InetAddress("127.0.0.1", 8080);
    sock->bind(addr);
    sock->listen();
    sock->setnonblocking();
    acceptChannel = new Channel(loop, sock->getFd());
    std::function<void()> cb = std::bind(&Acceptor::acceptConnection, this);
    acceptChannel->setCallback(cb);
    acceptChannel->enableReading();
    delete addr;
}

Acceptor::~Acceptor(){
    delete sock;
    delete acceptChannel;
}

void Acceptor::acceptConnection(){
    InetAddress *clnt_addr = new InetAddress();
    Socket *clnt_sock = new Socket(sock->accept(clnt_addr));
    std::cout << "new client fd " << clnt_sock->getFd()
        << "! IP: " << inet_ntoa(clnt_addr->getAddr().sin_addr)
        << " Port:" << ntohs(clnt_addr->getAddr().sin_port)
        << std::endl;
    clnt_sock -> setnonblocking();
    newConnectionCallback(clnt_sock);
    delete clnt_addr;
}

void Acceptor::setNewConnectionCallback(std::function<void(Socket*)> _cb){
    newConnectionCallback = _cb;
}