#ifndef _PH_MEMBER_
#define _PH_MEMBER_
#include <gmpxx.h>

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
        mpz_class get_x() const;
        mpz_class get_y() const;
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
        mpz_class x;                        // exp_mod_product / ((modulus - 1) / 2)
        mpz_class y;                        // x^(-1) mod (modulus - 1) / 2
        mpz_class modulus;                  //成员的模数
};
#endif