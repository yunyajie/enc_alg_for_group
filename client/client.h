#ifndef _CLIENT_H_
#define _CLIENT_H_

#include "../PH_Cipher/PH_Cipher.h"
#include "../buffer/buffer.h"
#include <arpa/inet.h>
#include <assert.h>
#include <unistd.h>

class Client{
    public:
        Client(int server_port, const char* server_ip);
        ~Client();
        void test();
        std::pair<std::string, std::string> sendAndreceive(const std::string& title, const std::string& content);      //向服务器发送消息并获取服务器回送的消息
    private:
        bool init();                        //初始化套接字和服务端地址
    private:
        const int server_port_;             //服务端端口号
        const char* server_ip_;             //服务端IP地址
        sockaddr_in server_addr_;           //服务端地址
        int client_sock_;                   //客户端套接字
        Buffer readBuf_;                    //读缓冲区
        Buffer writeBuf_;                   //写缓冲区
};


#endif