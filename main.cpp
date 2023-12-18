#include <iostream>
#include <vector>
#include "PH_Cipher/PH_Cipher.h"
#include "server/epoller.h"
#include "buffer/buffer.h"
#include <arpa/inet.h> // sockaddr_in
#include <fcntl.h>

//设置文件描述符非阻塞
int setnonblocking(int fd){
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

int main(){
    PH_Cipher server(2, 32);
    server.init();

    int port = 8000;

    //创建epoll
    Epoller epoller;
    //创建监听套接字
    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    assert(listenfd >= 0);
    //设置地址重用
    int flag = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));

    int ret = 0;
    //绑定地址
    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY); //服务器可绑定任意地址
    addr.sin_port = htons(port);
    ret = bind(listenfd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
    assert(ret >= 0);
    //将对监听套接字的读事件加入内核事件表
    uint32_t events = EPOLLIN;
    epoller.addFd(listenfd, events);

    //等待事件
    while(true){
        int numEvents = epoller.wait();
        if(numEvents < 0 && errno != EINTR){
            std::cout << "epoll failure!" << std::endl;
            break;
        }
        for(int i = 0; i < numEvents; i++){
            int sockfd = epoller.getEventFd(i);
            uint32_t events = epoller.getEvents(i);
            //处理新到的客户连接
            if(sockfd == listenfd){
                sockaddr_in client_addr;
                socklen_t client_addrlen = sizeof(client_addr);
                int connfd = accept(listenfd, reinterpret_cast<sockaddr*>(&client_addr), &client_addrlen);
                if(connfd < 0){
                    std::cout << "accept error: errno is:" << errno << std::endl;
                    continue;
                }
                
            }else if(events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)){
                
            }else if(events & EPOLLIN){
                //客户端输入事件
                Buffer buffer;
                int err;
                int len = buffer.ReadFd(sockfd, &err);
                if(len == -1){
                    //没有更多的数据可读
                    if(err == EAGAIN || err == EWOULDBLOCK){
                        break;
                    }else{
                        //发生错误
                        perror("read");
                        break;
                    }
                }else if(len == 0){
                    //客户端关闭连接
                    close(sockfd);
                    epoller.delFd(sockfd);
                    break;
                }else{
                    //处理读取数据
                    std::string str = buffer.RetrieveAllToStr();
                    std::cout << "Received data: " << str <<std::endl;
                }
            }
        }
    }

    close(listenfd);
    return 0;
}