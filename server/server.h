#ifndef _SERVER_H_
#define _SERVER_H_
#include "epoller.h"
#include "../log/log.h"
#include "../PH_Cipher/PH_Cipher.h"
#include "../conn/conn.h"
#include <memory>
#include <unordered_map>
#include <fcntl.h>
#include <gmpxx.h>

class Server{
    public:
        Server(int port, int keylevel = 30, int timeoutMs = 5, bool openLog = true, int logLevel = 0);
        ~Server();
        void start();               //启动
    private:
        bool initSock();                                //初始化监听文件描述符
        static int setFdNonblock(int fd);               //设置描述符非阻塞
        void addClient(int fd, sockaddr_in addr);       //添加新到来的客户端
        void closeConn(Conn* client);                   //关闭连接
        void dealListen();                              //处理新的连接
        void dealRead(Conn* client);                    //处理读事件
        void dealWrite(Conn* client);                   //处理写事件
        //读事件中需要处理的逻辑
        void onRead_allocation(Conn* client);           //成员注册
        int onRead_member_join(Conn* client);           //成员离开
        int onRead_member_leave(Conn* client);          //成员离开
        void onRead_error(Conn* client);                //非预期行为
        //写事件需要处理的逻辑
        void onWrite_newKey();                          //组密钥更新----向所有活跃组成员的写缓冲区写入新的组密钥的广播消息并触发其写事件

        void generate_new_gk();
    private:
        int port_;                              //服务器端口
        int listenfd_;                          //监听套接字
        int timeoutMs_;                         //超时事件 毫秒Ms
        std::unique_ptr<Epoller> epoller_;      //epoll 对象
        std::unique_ptr<PH_Cipher> ph_cipher_;  //加密算法对象
        bool isClose_;                          //是否关闭
        // uint32_t listenEvent_;                  //监听的文件描述符的事件
        uint32_t connEvent_;                    //连接的文件描述符的事件
        std::unordered_map<int, Conn> clients_;  //客户端连接的信息

        int key_leavel_;//组密钥位数
        mpz_class gk_;//组密钥
};


#endif