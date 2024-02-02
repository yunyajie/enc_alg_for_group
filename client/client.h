#ifndef _CLIENT_H_
#define _CLIENT_H_

#include "../buffer/buffer.h"
#include "../log/log.h"
#include "../cipher/XH_Cipher/Utility.h"
#include <arpa/inet.h>
#include <assert.h>
#include <unistd.h>
#include <gmpxx.h>

class Client{
    public:
        Client(int server_port, const char* server_ip, const char* userName, const char* passwd);
        ~Client();
        void start();                                                   //启动
        mpz_class decrypt();
        mpz_class get_gk();
    private:
        int readFd(std::pair<std::string, std::string>& message);       //每次读一个消息
        int writeFd();                                                  //将写缓冲区的所有消息写入套接字
        bool init();                        //初始化套接字和服务端地址
    private:
        bool isStop_;                       //是否连接成功可启动
        const int server_port_;             //服务端端口号
        const char* server_ip_;             //服务端IP地址
        sockaddr_in server_addr_;           //服务端地址
        int client_sock_;                   //客户端套接字
        Buffer readBuf_;                    //读缓冲区
        Buffer writeBuf_;                   //写缓冲区

        bool isregistered_;                 //是否已在系统注册
        bool isactive_;                     //是否加入活跃组
        mpz_class mod_;                     //模数---解密密钥
        mpz_class gk_cipher_;               //接收到的密文
        mpz_class r_;                       //接收到的当前会话的随机数
        mpz_class gk_;                      //组密钥

        std::string user_name_;             //用户名
        std::string passwd_;                //登录密码
};


#endif