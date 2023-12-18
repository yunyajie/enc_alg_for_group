#ifndef _CONN_H_
#define _CONN_H_

#include "../buffer/buffer.h"
#include <arpa/inet.h>

class Conn{
    public:
        Conn();
        ~Conn();
        int getfd();
        sockaddr_in getaddr();
        void init(int fd, const sockaddr_in& addr);
        void closeFd();
        ssize_t read(int* saveError);
        std::string getMessage();
        static bool isET;                       //默认是ET模式
        static std::atomic<int> userCount;      //总共的客户端连接数量
    private:
        int fd_;                    //连接文件描述符
        sockaddr_in addr_;          //客户端地址
        bool isClose_;
        Buffer readBuff_;           //读缓冲区
        Buffer writeBuff_;          //写缓冲区
};

#endif