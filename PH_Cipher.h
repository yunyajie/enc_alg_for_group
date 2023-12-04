#ifndef _PH_CIPHER_
#define _PH_CIPHER_
#include <iostream>
#include <vector>
#include <gmpxx.h>


class PH_Member{
    public:
        PH_Member(mpz_class enc_key, mpz_class modulus);
        mpz_class decrypt(mpz_class& ciphertext);    //解密
    private:
        mpz_class enc_key;                  //成员的加密密钥
        mpz_class dec_key;                  //成员的解密密钥
        mpz_class modulus;                  //成员的模数
};

class PH_Cipher{
    public:
        PH_Cipher(int m);
        mpz_class encrypt(mpz_class& message);       //加密
        int member_join(PH_Member joiner);           //成员加入
        int member_leave(PH_Member leaver);          //成员离开
    private:
        int m;                              //系统最大成员个数
        mpz_class mod_product;              //系统所有成员的模数乘积
        std::vector<PH_Member> members;     //所有成员
        mpz_class m_key;                    //主加密密钥
        mpz_class active_mod_product;       //所有活跃成员的模数乘积
};

#endif