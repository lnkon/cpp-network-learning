#include "../include/Connection.hpp"
#include "../include/Socket.hpp"
#include "../include/Channel.hpp"
#include <unistd.h>
#include <strings.h>
#include <iostream>

#define READ_BUFFER 1024

Connection::Connection(EventLoop *_loop, Socket *_sock):loop(_loop), sock(_sock), channel(nullptr){
    channel = new Channel(loop, sock->getFd());
    std::function<void()> cb = std::bind(&Connection::echo, this, sock->getFd());
    channel->setCallback(cb);
    channel->enableReading();
}

Connection::~Connection(){
    delete channel;
    delete sock;
}

void Connection::echo(int sockfd){
    char buf[READ_BUFFER];
    while(true){
        bzero(&buf, sizeof(buf));
        ssize_t bytes_read = read(sockfd, buf, sizeof(buf));
        if(bytes_read > 0){
            std::cout << "message from client fd " << sockfd << ": "<< buf << std::endl;
        }else if(bytes_read == -1 && errno == EINTR){
            std::cout << "continue reading";
            continue;
        }else if(bytes_read == -1 && ((errno == EAGAIN) || (errno == EWOULDBLOCK))){
            std::cout << "finish reading once, errno: " << errno << std::endl;
            break;
        }else if(bytes_read == 0){
            std::cout << "EOF, client fd " << sockfd << " disconnected" << std::endl;
            deleteConnectionCallback(sock);
            break;
        }
    }
}

void Connection::setDeleteConnectionCallback(std::function<void(Socket*)> _cb){
    deleteConnectionCallback = _cb;
}