# TCP Epoll Server

我们在上个demo中实现了一个简单的 TCP Echo Server，但是我们发现现在的服务器模型只能支持一对一的连接，这和我们需要实现的功能不符合。

因此我们引入 Epoll 来解决当前模型无法支持一对多的连接。

## 涉及的知识点

### epoll 
epoll 是 Linux 特有的 I/O 复用函数。epoll 内部使用一组函数来完成任务，同时epoll把需要观察的文件描述符上的事件添加到内核中的事件表里。

### 阻塞与非阻塞
阻塞代表程序到达阻塞语句处会暂停程序的执行，直至被要求的事件或条件的完成或者程序因为异常原因的终止。 

非阻塞代表程序不会因为某事件未发生而暂停，把当前的结果交给程序自己处理。

### ET 与 LT
ET 代表着边缘触发，类似数字电路中的从高电平到低电平转化或者从低电平到高电平转化的时刻，只有一瞬，对 ET 来说代表着事件被触发，通知程序事件被触发，产生变化，然后便不再通知。

LT 代表着水平触发， 与 ET 不同的是，触发后会持续提醒程序，直至被触发的事件被解决或程序终止。

## 服务端实现流程
![服务端实现流程](../使用%20epoll%20搭建支持多客户端连接的TCP%20Echo%20server/img/Epoll_Server流程图.png)

## epoll 部分代码实现

### 创建 Epoll 实例
因为 epoll 中的事件是添加在内核中的，所以我们需要一个额外的文件描述符来标识这个事件表
```cpp
#include <sys/epoll.h>
int epfd = epoll_create1(0);
errif(
    epfd == -1,
    "epoll create error"
)
```

### 创建 设置非阻塞 函数
由于需要检测多个事件的触发，因此所有内部事件都不应该阻塞程序的运行
```cpp
# include <fcntl.h>
void setnonblocking(int fd){
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fd = fcntl(fd, F_SETFL, new_option);
}
```

### 设置监听 socket
由于 ET 触发模式需要在一次处理时间内处理完所有事情，因此当多个客户端同时尝试连接时，服务端只会为第一个客户端服务，因此我们需要将监听socket的事件触发模式设置为 LT， 持续提醒，直至所有服务端的连接完成。
```cpp
struct epoll_event ev;
bzero(&ev, sizeof(ev));
ev.data.fd = sockfd;
ev.events = EPOLLIN // 不设置默认为 LT
setnonblocking(sockfd);
```

### 将监听 socket 添加到 epoll 中
```cpp
epoll_ctl(epfd, EPOLL_CTL_ADD, sockfd, &ev);
bzero(&ev, sizeof(ev));
```

### 创建监听事件数组并设置  epoll_wait()
```cpp
创建一个用于监听事件数组，通过 epoll_wait()记录触发事件的个数，并将事件添加进数组，便于对事件进行操作。
struct epoll_event events[MAX_EVENTS];
bzero(&events, sizeof(events));

while(true){
    int nfds = epoll_wait(epfd, events, MAXEVENTS， -1); 
    // 将 epfd 中触发的事件添加进 events 数组中，同时设置最大监听数量以及阻塞等待
    // -1 代表阻塞等待
}
```

## 区分监听的事件的源头
我们注意到在当前并没有设置可读事件发生时是来自于监听 socket 还是 已连接的客户端有消息。因此我们需要进行区分。

### 监听 socket 被触发

在 epoll 返回触发事件的文件描述符传回后，对文件描述符进行判断，判断是否来自于监听 socket 本身，如果来自本身则需要创建用于连接的客户端socket
```cpp
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
}
```

同时，我们仍然需要对新创建出的用于连接的socket添加可被触发的事件，为其添加可读事件边缘出发，以及不阻塞。
```cpp
bzero(&ev, sizeof(ev));
ev.data.fd = clnt_sockfd;
ev.events = EPOLLIN | EPOLLET;
setnonblocking(clnt_sockfd);
epoll_ctl(epfd, EPOLL_CTL_ADD, clnt_sockfd, &ev);
```

### 来自客户端的可读事件
创建读取缓冲区，用于读取内部通讯 socket 中的数据
```cpp
char buf[READ_BUFFER];

bzero(&buf, sizeof(buf));
ssize_t read_bytes = read(events[i].data.fd, buf, sizeof(buf));
```

> 值得注意的是：由于我们采用 ET 边缘触发模式，并且设置了定长字符读取，我们需要在一次读取操作内将数据完全读取出来，否则将面临数据丢失的风险！

创建循环，进行读取操作，并尝试区分四种基本情况：
- 读取到数据
- 对方关闭连接
- read 操作因为某次操作被迫终止，尝试重新进行读取
- 读取完成


我们将区分四种不同的情况。

正常读取到数据：
```cpp
if(read_bytes > 0){
    std::cout << "message from client fd " << events[i].data.fd
        << ": " << buf << std::endl;
    write(events[i].data.fd, buf, sizeof(buf));
}
```

客户端关闭连接，传输通道关闭，回收分配给客户端的文件描述符：
```cpp
else if(read_bytes == 0){
    close(events[i].data.fd);
    break;
}
```

read 操作因为某次操作被迫终止，尝试重新进行读取：
```cpp
else if(read_bytes == -1 && errno == EINTR){
    std::cout << "continue reading";
    continue;
}
```

正常结束，没有可读数据：
```cpp
else if(read_bytes == -1 && ((errno == EAGAIN) || (errno == EWOULDBLOCK))){
    std::cout << "finish reading noce, errno: " << errno << std::endl;  
    break;
}    
```

程序结束后，收回服务端的文件描述符。
```cpp
close(sockfd);
```

> 客户端与前一个demo的源码一样，本文的源码见[此处](/demo_2%20使用%20epoll%20搭建支持多客户端连接的TCP%20Echo%20server/Epoll_TCP_Server/)