#include "../include/Server.hpp"
#include "../include/Socket.hpp"
#include "../include/Acceptor.hpp"
#include "../include/Connection.hpp"
#include <functional>

Server::Server(EventLoop *_loop):loop(_loop), acceptor(nullptr){
    acceptor = new Acceptor(loop);
    std::function<void(Socket*)> cb = std::bind(&Server::newConnection, this, std::placeholders::_1);
    acceptor->setNewConnectionCallback(cb);
}

Server::~Server(){
    delete acceptor;
}

void Server::newConnection(Socket* sock){
    Connection *conn = new Connection(loop, sock);
    std::function<void(Socket*)> cb = std::bind(&Server::deleteConnection, this, std::placeholders::_1);
    conn->setDeleteConnectionCallback(cb);
    connections[sock->getFd()] = conn;
}

void Server::deleteConnection(Socket* sock){
    Connection *conn = connections[sock->getFd()];
    connections.erase(sock->getFd());
    delete conn;
}