#include "epoller.h"

//创建一个 epoll 句柄
Epoller::Epoller(int maxEvent):epollFd_(epoll_create(512)), events_(maxEvent){
    assert(epollFd_ >= 0 && events_.size() > 0);
}

Epoller::~Epoller(){
    close(epollFd_);    //关闭句柄
}

//注册新的 fd 到 epollfd 中
bool Epoller::addFd(int fd, uint32_t events){
    if(fd < 0) return false;
    epoll_event ev = {0};
    ev.data.fd = fd;
    ev.events = events;
    return 0 == epoll_ctl(epollFd_, EPOLL_CTL_ADD, fd, &ev);
}

//修改 fd 上注册到 epollfd 中的事件
bool Epoller::modFd(int fd, uint32_t events){
    if(fd < 0) return false;
    epoll_event ev = {0};
    ev.data.fd = fd;
    ev.events = events;
    return 0 == epoll_ctl(epollFd_, EPOLL_CTL_MOD, fd, &ev);
}

//删除 epollfd 上注册的 fd
bool Epoller::delFd(int fd){
    if(fd < 0) return false;
    epoll_event ev = {0};
    return 0 == epoll_ctl(epollFd_, EPOLL_CTL_DEL, fd, &ev);
}

//从内核获取事件集合
int Epoller::wait(int timeoutMs){
    return epoll_wait(epollFd_, &events_[0], static_cast<int>(events_.size()), timeoutMs);
}

//获取第 i 个事件的文件描述符
int Epoller::getEventFd(size_t i) const{
    assert(i < events_.size() && i >= 0);
    return events_[i].data.fd;
}

//获取第 i 个事件实际发生的事件
uint32_t Epoller::getEvents(size_t i) const{
    assert(i < events_.size() && i >= 0);
    return events_[i].events;
}