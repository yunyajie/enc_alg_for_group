#ifndef _CIPHER_MEMBER_H_
#define _CIPHER_MEMBER_H_
#include<gmpxx.h>

enum class Cipher_Method{
    PH_CIPHER,
    XH_CIPHER
};

class Cipher_Member{
    public:
        Cipher_Member(){};
        virtual mpz_class decrypt(const mpz_class& ciphertext) = 0;    //解密
        virtual mpz_class get_modulus() const = 0;
        virtual void set_x(mpz_class& x) = 0;
        virtual void set_y(mpz_class& y) = 0;
        virtual mpz_class get_x() const = 0;
        virtual mpz_class get_y() const = 0;
        virtual bool isRegistered() const = 0;
        virtual bool isActive() const = 0;
        virtual void registered() = 0;
        virtual void active() = 0;
        virtual void deactive() = 0;
        virtual ~Cipher_Member() = default;
};

#endif