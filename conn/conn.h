#ifndef _CONN_H_
#define _CONN_H_

#include "../buffer/buffer.h"
#include "../PH_Cipher/PH_Cipher.h"
#include "../pool/sqlconnpool.h"
#include "../pool/sqlconnRAII.h"
#include "../log/log.h"
#include <arpa/inet.h>
#include <mysql/mysql.h>

class Conn{
    public:
        Conn();
        ~Conn();
        int getfd();
        sockaddr_in getaddr();
        void init(int fd, const sockaddr_in& addr);
        void closeFd();
        PH_Member& getph_member();              //初始化其加密参数
        ssize_t read(int* saveError);
        ssize_t write(int* saveError);
        int getMessage(std::pair<std::string, std::string>& message);
        void addMessage(const std::string& title, const std::string& content);
        std::string GetReadStrNotRetrive();

        static bool isET;                       //默认是ET模式
        static std::atomic<int> userCount;      //总共的客户端连接数量
    //private:
        void writeToBuff(std::string str);      //向写缓冲区写入

        bool userVerify(const std::string& name, const std::string& pwd, bool isLogin);     //isLogin  注册是0 登录是1
    private:
        int fd_;                    //连接文件描述符
        sockaddr_in addr_;          //客户端地址
        bool isClose_;
        Buffer readBuff_;           //读缓冲区
        Buffer writeBuff_;          //写缓冲区
        PH_Member ph_member_;       //加密算法信息

        //用户状态
};

#endif