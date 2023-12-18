#ifndef _PH_CIPHER_
#define _PH_CIPHER_
#include <iostream>
#include <vector>
#include <algorithm>
#include <chrono>
#include <unordered_map>
#include <gmpxx.h>

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

class PH_Member{
    public:
        PH_Member();
        PH_Member(mpz_class enc_key, mpz_class modulus);
        mpz_class decrypt(const mpz_class& ciphertext);    //解密
        mpz_class get_enc_key() const;
        mpz_class get_dec_key() const;
        mpz_class get_modulus() const;
        ~PH_Member();
    private:
        mpz_class enc_key;                  //成员的加密密钥
        mpz_class dec_key;                  //成员的解密密钥
        mpz_class modulus;                  //成员的模数
};

class PH_Cipher{
    public:
        PH_Cipher(int m, int bit_length);
        void init();                                       //系统初始化
        int allocation(PH_Member& new_register);           //新成员注册
        mpz_class encrypt(const mpz_class& message);       //加密
        int member_join(const PH_Member& joiner);           //成员加入
        int member_leave(const PH_Member& leaver);          //成员离开
        int sys_entend();                                  //备用密钥分配完毕，系统需要扩展，计划扩展为原来规模的2倍
        ~PH_Cipher();
        
    private:
        void init_xy();                     //初始化 x 和 y
        void init_lcm_modproduct();         //初始化 lcm 和 mod_product
        void sys_init();                    //系统初始化
        void generate_safe_prime(mpz_class& safe_prime, const unsigned long int reps);
        std::vector<PH_Member> init_members(int n);        //获取新的 n 个可用成员位置
        void master_key_init();             //初始化 m_key
    private:
        int bit_length;                     //密钥长度
        int m;                              //系统最大成员个数
        std::vector<PH_Member> members;     //所有成员
        std::unordered_map<mpz_class, int, mpz_class_hash> mod_map; //成员和 members 下标之间的映射， 方便成员加入和离开的管理
        std::vector<mpz_class> x;           // lcm / m_i
        std::vector<mpz_class> y;           // x_i 模 m_i 的逆
        mpz_class mod_product;              //系统所有成员的模数乘积
        mpz_class lcm;                      // 所有 m_i 的最小公倍数
        int available;         //标记下一个待分配成员位置
        std::vector<int> active_members_index;    //活跃成员下标
        mpz_class m_key;                    //主加密密钥
        mpz_class active_mod_product;       //所有活跃成员的模数乘积
        mpz_class active_lcm;               //所有活跃成员 m_i 的最小公倍数

        mpz_class modulus_lower_bound;      //生成模数时的下界
};

#endif