#include "../include/EventLoop.hpp"
#include "../include/Epoll.hpp"
#include "../include/Channel.hpp"
#include <vector>

EventLoop::EventLoop():ep(nullptr), quit(false){
    ep = new Epoll();
}

EventLoop::~EventLoop(){
    delete ep;
}

void EventLoop::loop(){
    while(!quit){
        std::vector<Channel*> chs;
        chs = ep->poll();
        for(auto it =  chs.begin(); it != chs.end(); ++it){
            (*it)->handleEvent();
        }
    }
}
void EventLoop::updateChannel(Channel *ch){
    ep->updateChannel(ch);
}