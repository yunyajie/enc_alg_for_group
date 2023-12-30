#ifndef _CLIENT_H_
#define _CLIENT_H_

#include "../PH_Cipher/PH_Cipher.h"
#include "../buffer/buffer.h"
#include "../log/log.h"
#include <arpa/inet.h>
#include <assert.h>
#include <unistd.h>
#include <gmpxx.h>

class Client{
    public:
        Client(int server_port, const char* server_ip);
        ~Client();
        void start();                                                   //启动
        int readFd(std::pair<std::string, std::string>& message);       //每次读一个消息
        int writeFd();                                                  //将写缓冲区的所有消息写入套接字
        mpz_class decrypt(const mpz_class& ciphertext);
    private:
        bool init();                        //初始化套接字和服务端地址
    private:
        bool isStop_;                       //是否连接成功可启动
        const int server_port_;             //服务端端口号
        const char* server_ip_;             //服务端IP地址
        sockaddr_in server_addr_;           //服务端地址
        int client_sock_;                   //客户端套接字
        Buffer readBuf_;                    //读缓冲区
        Buffer writeBuf_;                   //写缓冲区

        bool isregistered_;                  //是否已注册
        bool isactive_;                      //是否加入活跃组
        mpz_class mod_;                     //模数
        mpz_class dec_key_;                 //解密密钥
        mpz_class gk_cipher_;
        mpz_class gk_;
};


#endif