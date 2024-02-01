#include "PH_Member.h"

//PH_Member 的实现
PH_Member::PH_Member():isregistered(false), isactive(false){
    //空构造
}

PH_Member::PH_Member(mpz_class enc_key, mpz_class modulus):enc_key(enc_key), modulus(modulus), isregistered(false), isactive(false){
    //初始化解密密钥
    mpz_invert(dec_key.get_mpz_t(), enc_key.get_mpz_t(), static_cast<mpz_class>(modulus - 1).get_mpz_t());
}

mpz_class PH_Member::decrypt(const mpz_class& ciphertext){
    mpz_class message;
    mpz_powm(message.get_mpz_t(), ciphertext.get_mpz_t(), dec_key.get_mpz_t(), modulus.get_mpz_t());
    return message;
}

mpz_class PH_Member::get_enc_key() const{
    return enc_key;
}

mpz_class PH_Member::get_dec_key() const{
    return dec_key;
}

mpz_class PH_Member::get_modulus() const{
    return modulus;
}

bool PH_Member::isRegistered() const{
    return isregistered;
}

bool PH_Member::isActive() const{
    return isactive;
}

void PH_Member::registered(){
    isregistered = true;
}

void PH_Member::active(){
    isactive = true;
}

void PH_Member::deactive(){
    isactive = false;
}

void PH_Member::set_x(mpz_class& x){
    this->x = x;
}

void PH_Member::set_y(mpz_class& y){
    this->y = y;
}

mpz_class PH_Member::get_x() const{
    return this->x;
}

mpz_class PH_Member::get_y() const{
    return this->y;
}

PH_Member::~PH_Member(){}

