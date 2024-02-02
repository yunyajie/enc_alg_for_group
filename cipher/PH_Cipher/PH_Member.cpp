#include "PH_Member.h"

//PH_Member 的实现
PH_Member::PH_Member():isregistered_(false), isactive_(false){
    //空构造
}

PH_Member::PH_Member(mpz_class enc_key, mpz_class modulus):enc_key_(enc_key), modulus_(modulus), isregistered_(false), isactive_(false){
    //初始化解密密钥
    mpz_invert(dec_key_.get_mpz_t(), enc_key_.get_mpz_t(), static_cast<mpz_class>(modulus_ - 1).get_mpz_t());
}

mpz_class PH_Member::decrypt(const mpz_class& ciphertext){
    mpz_class message;
    mpz_powm(message.get_mpz_t(), ciphertext.get_mpz_t(), dec_key_.get_mpz_t(), modulus_.get_mpz_t());
    return message;
}

mpz_class PH_Member::get_enc_key() const{
    return enc_key_;
}

mpz_class PH_Member::get_dec_key() const{
    return dec_key_;
}

mpz_class PH_Member::get_modulus() const{
    return modulus_;
}

bool PH_Member::isRegistered() const{
    return isregistered_;
}

bool PH_Member::isActive() const{
    return isactive_;
}

void PH_Member::registered(){
    isregistered_ = true;
}

void PH_Member::active(){
    isactive_ = true;
}

void PH_Member::deactive(){
    isactive_ = false;
}

void PH_Member::set_x(mpz_class& x){
    x_ = x;
}

void PH_Member::set_y(mpz_class& y){
    y_ = y;
}

mpz_class PH_Member::get_x() const{
    return x_;
}

mpz_class PH_Member::get_y() const{
    return y_;
}

