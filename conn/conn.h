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
        PH_Member& getph_member();                                      //获取其加密参数内容
        ssize_t read(int* saveError);                                   //从文件描述符读取数据到输入缓冲区，直到把所有数据读完，返回读到的字节数或 -1
        ssize_t write(int* saveError);                                  //向文件描述符写入输出缓冲区中所有数据
        int getMessage(std::pair<std::string, std::string>& message);   //从输入缓冲区获取一条消息
        void addMessage(const std::string& title, const std::string& content);  //向输出缓冲区添加一条消息
        std::string GetReadStrNotRetrive();                             //查看待发送的或待读取的消息

        static bool isET;                       //默认是ET模式
        static std::atomic<int> userCount;      //总共的客户端连接数量
    //private:
        void writeToBuff(std::string str);      //向写缓冲区写入

        bool userVerify(const std::string& name, const std::string& pwd, bool isLogin);     //isLogin  注册是0 登录是1

        //获取用户状态
        std::string getUserName();
        int getUserDbid();
    private:
        int fd_;                    //连接文件描述符
        sockaddr_in addr_;          //客户端地址
        bool isClose_;
        Buffer readBuff_;           //读缓冲区
        Buffer writeBuff_;          //写缓冲区
        PH_Member ph_member_;       //加密算法信息

        //用户状态
        bool islogin_;              //是否成功登录
        std::string userName_;      //用户账户姓名
        std::string passwd_;        //用户密码
        int userDbid_;              //用户数据库id
};

#endif