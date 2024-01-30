#ifndef _XH_MEMBER_
#define _XH_MEMBER_
#include<gmpxx.h>
class XH_Member{
    public:
        XH_Member(mpz_class& modulus);
        ~XH_Member();
        mpz_class decrypt(const mpz_class& ciphertext);    //解密
        mpz_class get_modulus() const;
        mpz_class get_x() const;
        mpz_class get_y() const;
        mpz_class get_r() const;
        void set_r(mpz_class& r);
        void set_x(mpz_class& x);
        void set_y(mpz_class& y);
        bool isRegistered() const;
        bool isActive() const;
        void registered();
        void active();
        void deactive();
    private:
        bool _isregistered;                  //是否已注册
        bool _isactive;                      //是否加入活跃组
        mpz_class _r;                        //CRT 中对应的余数
        mpz_class _x;                        // mod_product / modulus
        mpz_class _y;                        // x^(-1) mod modulus
        mpz_class _m;                        //成员的模数------也是解密的密钥
};
#endif