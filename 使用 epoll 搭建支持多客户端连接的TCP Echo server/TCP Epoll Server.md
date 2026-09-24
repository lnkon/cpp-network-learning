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
