#ifndef _PH_CIPHER_
#define _PH_CIPHER_
#include <iostream>
#include <vector>
#include <algorithm>
#include <chrono>
#include <unordered_map>
#include <unordered_set>
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
        void set_x(mpz_class& x);
        void set_y(mpz_class& y);
        mpz_class get_x();
        mpz_class get_y();
        bool isRegistered() const;
        bool isActive() const;
        void registered();
        void active();
        void deactive();
        ~PH_Member();
    private:
        mpz_class enc_key;                  //成员的加密密钥
        mpz_class dec_key;                  //成员的解密密钥
        bool isregistered;                  //是否已注册
        bool isactive;                      //是否加入活跃组
        mpz_class x;                        // lcm / (modulus - 1)
        mpz_class y;                        // x^(-1) mod (modulus - 1) / 2
        mpz_class modulus;                  //成员的模数
};

class PH_Cipher{
    public:
        PH_Cipher(int m = 2, int bit_length = 32);
        int allocation(PH_Member& new_register);           //新成员注册
        mpz_class encrypt(const mpz_class& message);       //加密
        int member_join(PH_Member& joiner);           //成员加入
        int member_leave(PH_Member& leaver);          //成员离开
        int active_size();
        ~PH_Cipher();
        
    private:
        int sys_extend();                   //备用密钥分配完毕，系统需要扩展，计划扩展为原来规模的2倍
        void init_xy();                     //初始化 x 和 y
        void init_lcm_modproduct();         //初始化 lcm 和 mod_product
        void sys_init();                    //系统初始化
        void generate_safe_prime(mpz_class& safe_prime, const unsigned long int reps);
        std::vector<PH_Member> init_members(int n);        //获取新的 n 个可用成员位置
        void master_key_init();             //初始化 m_key
    private:
        int bit_length;                     //密钥长度
        int m;                              //系统最大成员个数

        std::unordered_map<mpz_class, PH_Member, mpz_class_hash> members;   //系统所有成员集合
        std::unordered_set<mpz_class, mpz_class_hash> available;            //可分配成员集合
        std::unordered_set<mpz_class, mpz_class_hash> active_members;       //活跃组成员集合

        mpz_class mod_product;              //系统所有成员的模数乘积
        mpz_class lcm;                      // 所有 m_i 的最小公倍数
        mpz_class m_key;                    //主加密密钥
        mpz_class active_mod_product;       //所有活跃成员的模数乘积
        mpz_class active_lcm;               //所有活跃成员 m_i 的最小公倍数

        mpz_class modulus_lower_bound;      //生成模数时的下界
};

#endif