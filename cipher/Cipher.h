#ifndef _CIPHER_
#define _CIPHER_
#include <gmpxx.h>
#include "Cipher_Member.h"

class Cipher{
    public:
        Cipher(){};
        virtual int allocation(Cipher_Member& new_register) = 0;    //新成员注册分配密钥
        virtual mpz_class encrypt(const mpz_class& message) = 0;    //加密
        virtual int member_join(Cipher_Member& joiner) = 0;         //成员加入活跃组
        virtual int member_leave(Cipher_Member& leaver) = 0;        //成员离开活跃组
        virtual int active_size() = 0;                              //活跃组规模
        virtual int sys_size() = 0;                                 //系统规模
        virtual bool sys_init_fromDb() = 0;                         //从数据库初始化    在 server 将连接池启动之后再初始化
        virtual ~Cipher() = default;
};

#endif