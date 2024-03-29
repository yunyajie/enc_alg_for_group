#ifndef _XH_CIPHER_
#define _XH_CIPHER_
#include <iostream>
#include <vector>
#include <algorithm>
#include <chrono>
#include <unordered_map>
#include <unordered_set>
#include <gmpxx.h>
#include "../Cipher.h"
#include "Utility.h"
#include "XH_Member.h"
#include "../../log/log.h"
#include "../../pool/sqlconnpool.h"
#include "../../pool/sqlconnRAII.h"

//自定义哈希函数对象
struct mpz_class_hash{
    std::size_t operator()(const mpz_class& key) const {
        //使用GMP库提供的函数获取 mpz_class 对象的字符串表示
        std::string keyString = key.get_str();
        //使用 std::hash 对字符串进行哈希
        std::hash<std::string> stringHash;
        //返回字符串的哈希值
        return stringHash(keyString);
    }
};

class XH_Cipher : public Cipher{
    public:
        XH_Cipher(int m = 2, int bit_length = 257);
        int allocation(Cipher_Member& new_register);        //新成员注册分配密钥
        mpz_class encrypt(const mpz_class& message);    //加密
        int member_join(Cipher_Member& joiner);             //成员加入活跃组
        int member_leave(Cipher_Member& leaver);            //成员离开活跃组
        int active_size();                              //活跃组规模
        mpz_class get_r();                              //获取当前阶段的随机数
        int sys_size();                                 //系统规模
        bool sys_init_fromDb();                         //从数据库初始化    在 server 将连接池启动之后再初始化
        ~XH_Cipher() = default;
        
    private:
        int sys_extend_Db();                //备用密钥分配完毕，系统需要扩展，计划扩展为原来规模的2倍，并将新的密钥加入到数据库中
        void init_xy();                     //初始化 x 和 y，x * y mod m_i = 1
        void init_modproduct();             //初始化 mod_product 和 exp_mod_product
        void generate_prime(mpz_class& result_prime); //生成一个大于 _modulus_lower_bound 的素数
        std::vector<XH_Member> init_members(int n);        //获取 n 个新的可用密钥对
    private:
        int _bit_length;                     //密钥长度
        int _m;                              //系统最大成员个数

        std::unordered_map<mpz_class, XH_Member, mpz_class_hash> _members;   //系统所有成员集合
        std::unordered_set<mpz_class, mpz_class_hash> _available;            //可分配成员集合
        std::unordered_set<mpz_class, mpz_class_hash> _active_members;       //活跃组成员集合

        mpz_class _active_mod_product;      //所有活跃成员模数乘积
        mpz_class _mod_product;             //所有注册成员的模数乘积   m_1 ... *m_n

        mpz_class _r;                       //当前选择的随机数，每次加密前选择
        mpz_class _modulus_lower_bound;     //生成新的安全素数模数时的下界
};

#endif