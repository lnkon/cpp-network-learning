#include "utils/util.h"
#include <sys/socket.h>
#include <string.h>
#include <strings.h>
#include <arpa/inet.h>
#include <iostream>
#include <unistd.h>

#define READ_BUFFER 1024

int main(){
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    errif(sockfd == -1, "socket create error");


    struct sockaddr_in serv_addr;
    bzero(&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(8080);
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    errif(
        bind(sockfd, (sockaddr*)&serv_addr, sizeof(serv_addr)) == -1,
        "socket bind error"
    );

    errif(
        listen(sockfd, SOMAXCONN) == -1,
        "socket listen error"
    );

    struct sockaddr_in clnt_addr;
    socklen_t clnt_addr_len = sizeof(clnt_addr);
    bzero(&clnt_addr, sizeof(clnt_addr));

    int clnt_sockfd = accept(sockfd, (sockaddr*)&clnt_addr, &clnt_addr_len);
    errif(
        clnt_sockfd == -1,
        "socket accept error"
    );
    
    std::cout << "new client fd: " << clnt_sockfd 
        << "! IP: " << inet_ntoa(clnt_addr.sin_addr) 
        << " Port:" << htons(clnt_addr.sin_port) << std::endl;  

        
    char buf[READ_BUFFER];
    while(true){
        bzero(&buf, sizeof(buf));
        ssize_t read_bytes = read(clnt_sockfd, buf, sizeof(buf));
        if(read_bytes > 0){
            std::cout << "message from client fd " << clnt_sockfd
                << ": " << buf << std::endl;
            write(clnt_sockfd, buf, sizeof(buf));
        }else if(read_bytes == 0){
            std::cout << "client fd " << clnt_sockfd << " disconnected" << std::endl;
            break;
        }else if(read_bytes == -1){
            close(clnt_sockfd);
            errif(
                true,
                "socket read error"
            );
        }
    }
    close(clnt_sockfd);
}