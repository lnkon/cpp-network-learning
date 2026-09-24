#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <strings.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include "../include/util.hpp"

#define MAX_EVENT 1024
#define READ_BUFFER 1024

void setnonblocking(int fd);

int main(){
    int sockfd  = socket(AF_INET, SOCK_STREAM, 0);
    errif(
        sockfd == -1,
        "socket create error"
    );

    struct sockaddr_in serv_addr;
    bzero(&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(8080);
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    errif(
        bind(sockfd, (sockaddr*)&serv_addr, sizeof(serv_addr)),
        "socket bind error"
    );

    errif(
        listen(sockfd, SOMAXCONN) == -1,
        "socket listen error"
    );

    int epfd = epoll_create1(0);
    errif(
        epfd == -1,
        "epoll create error"
    );

    struct epoll_event events[MAX_EVENT];
    struct epoll_event ev;
    ev.data.fd = sockfd;
    ev.events = EPOLLIN;
    epoll_ctl(epfd, EPOLL_CTL_ADD, sockfd, &ev);

    while(true){
        int nfds  = epoll_wait(epfd, events, MAX_EVENT, -1);
        for(int i = 0; i < nfds; i++){
            if(events[i].data.fd == sockfd){
                struct sockaddr_in clnt_addr;
                bzero(&clnt_addr, sizeof(clnt_addr));
                socklen_t clnt_addr_len = sizeof(clnt_addr);
                int clnt_sockfd = accept(sockfd, (sockaddr*)&clnt_addr, &clnt_addr_len);
                errif(
                    clnt_sockfd == -1,
                    "socker accept error"
                );
                std::cout << "new client fd: " << clnt_sockfd 
                    << "! IP: " << inet_ntoa(clnt_addr.sin_addr) 
                    << " Port:" << htons(clnt_addr.sin_port) << std::endl;
                
                bzero(&ev, sizeof(ev));
                ev.data.fd = clnt_sockfd;
                ev.events = EPOLLIN | EPOLLET;
                setnonblocking(clnt_sockfd);
                epoll_ctl(epfd, EPOLL_CTL_ADD, clnt_sockfd, &ev);
            }else if(events[i].events & EPOLLIN){
                char buf[READ_BUFFER];
                while(true){
                    bzero(&buf, sizeof(buf));
                    ssize_t read_bytes = read(events[i].data.fd, buf, sizeof(buf));
                    if(read_bytes > 0){
                        std::cout << "message from client fd " << events[i].data.fd
                            << ": " << buf << std::endl;
                        write(events[i].data.fd, buf, sizeof(buf));
                    }else if(read_bytes == 0){
                        close(events[i].data.fd);
                        break;
                    }else if(read_bytes == -1 && errno == EINTR){
                        std::cout << "continue reading";
                        continue;
                    }else if(
                        read_bytes == -1 &&
                        (
                            (errno == EAGAIN) || 
                            (errno == EWOULDBLOCK)
                        )
                    ){
                        std::cout << "finish reading noce, errno: " << errno << std::endl;  
                        break;
                    }
                }
            }
        }
    }
    close(sockfd);
}

void setnonblocking(int fd){
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
}
