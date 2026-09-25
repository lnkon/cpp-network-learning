#include "utils/util.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <strings.h>
#include <unistd.h>
#include <iostream>

#define WRITE_BUFFER 1024

int main(){
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
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
        connect(sockfd, (sockaddr*)&serv_addr, sizeof(serv_addr)) == -1,
        "socket connect error"
    );
    
    char buf[WRITE_BUFFER];
    while(true){
        bzero(&buf, sizeof(buf));
        std::cin >> buf;
        ssize_t write_bytes = write(sockfd, buf, sizeof(buf));
        bzero(&buf, sizeof(buf));
        ssize_t read_bytes = read(sockfd, buf, sizeof(buf));
        if(read_bytes > 0){
            std::cout << "message from server fd " << sockfd
                << ": " << buf << std::endl;
        }else if(read_bytes == 0){
            std::cout << "server fd " << sockfd << " disconnected" << std::endl;
            break;
        }else if(read_bytes == -1){
            close(sockfd);
            errif(
                true,
                "socket read error"
            );
        }
    }
    close(sockfd);
}