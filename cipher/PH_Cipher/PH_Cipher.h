#ifndef _PH_CIPHER_
#define _PH_CIPHER_
#include <iostream>
#include <vector>
#include <algorithm>
#include <chrono>
#include <unordered_map>
#include <unordered_set>
#include <gmpxx.h>
#include "PH_Member.h"
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

class PH_Cipher{
    public:
        PH_Cipher(int m = 2, int bit_length = 32);
        int allocation(PH_Member& new_register);        //新成员注册分配密钥
        mpz_class encrypt(const mpz_class& message);    //加密
        int member_join(PH_Member& joiner);             //成员加入活跃组
        int member_leave(PH_Member& leaver);            //成员离开活跃组
        int active_size();                              //活跃组规模
        int sys_size();                                 //系统规模

        bool sys_init_fromDb();                         //从数据库初始化    在 server 将连接池启动之后再初始化
        ~PH_Cipher();
        
    private:
        int sys_extend();                   //备用密钥分配完毕，系统需要扩展，计划扩展为原来规模的2倍----测试用，未连接数据库
        int sys_extend_Db();                //备用密钥分配完毕，系统需要扩展，计划扩展为原来规模的2倍，并将新的密钥加入到数据库中
        void init_xy();                     //初始化 x 和 y，x * y mod m_i = 1
        void init_modproduct();             //初始化 mod_product 和 exp_mod_product
        void sys_init();                    //系统初始化----测试用，未连接数据库
        void generate_safe_prime(mpz_class& safe_prime, const unsigned long int reps);
        std::vector<PH_Member> init_members(int n);        //获取 n 个新的可用密钥对
        void master_key_init();             //初始化 m_key 和 m_key_info
    private:
        int bit_length;                     //密钥长度
        int m;                              //系统最大成员个数

        std::unordered_map<mpz_class, PH_Member, mpz_class_hash> members;   //系统所有成员集合
        std::unordered_set<mpz_class, mpz_class_hash> available;            //可分配成员集合
        std::unordered_set<mpz_class, mpz_class_hash> active_members;       //活跃组成员集合

        mpz_class mod_product;              //系统所有成员的安全素数模数 p_i 乘积 (p_i = 2 * m_i + 1)
        mpz_class active_mod_product;       //所有活跃成员模数乘积
        mpz_class exp_mod_product;          //指数上模数乘积   2*m_1 ... *m_n
        mpz_class active_exp_mod_product;   //所有活跃成员指数上的模数乘积
        mpz_class m_key_info;               //活跃组成员密钥信息累加和
        mpz_class m_key;                    //主加密密钥  m_key = m_key_info % active_exp_mod_product

        mpz_class modulus_lower_bound;      //生成新的安全素数模数时的下界
};

#endif