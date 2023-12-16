#ifndef _EPOLLER_H_
#define _EPOLLER_H_
#include <unistd.h> //close()
#include <sys/epoll.h> //epoll_ctl()
#include <vector>
#include <assert.h>


//实现对 epoll 的封装
class Epoller{
public:
    explicit Epoller(int maxEvent = 1024);      //创建一个 epoll 句柄，maxEvent 最多可以监听的事件的数量
    ~Epoller();
    bool addFd(int fd, uint32_t events);        //注册新的 fd 到 epollfd 中
    bool modFd(int fd, uint32_t events);        //修改 fd 上注册到 epollfd 中的事件
    bool delFd(int fd);                         //删除 epollfd 上注册的 fd
    int wait(int timeoutMs = -1);               //从内核获取事件集合，epoll_wait 并使其阻塞 timeoutMs 毫秒   默认 -1 为永久阻塞直到有事件到达   0 表示不阻塞
    int getEventFd(size_t i) const;             //获取第 i 个事件的文件描述符
    uint32_t getEvents(size_t i) const;         //获取第 i 个事件实际发生的事件
private:
    int epollFd_;                               //epoll 的文件描述符，epoll_create() 的返回值
    std::vector<struct epoll_event> events_;    //epoll_wait() 调用后返回的事件存放的数组
};

#endif