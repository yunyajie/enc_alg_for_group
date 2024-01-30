#include "XH_Member.h"
XH_Member::XH_Member(mpz_class& modulus):_m(modulus){
}

mpz_class decrypt(const mpz_class& ciphertext){
    
}

mpz_class XH_Member::get_modulus() const{
    return _m;
}

mpz_class XH_Member::get_x() const{
    return _x;
}

mpz_class XH_Member::get_y() const{
    return _y;
}

mpz_class XH_Member::get_r() const{
    return _r;
}

bool XH_Member::isRegistered() const{
    return _isregistered;
}

bool XH_Member::isActive() const{
    return _isactive;
}

void XH_Member::registered(){
    _isregistered = true;
}

void XH_Member::active(){
    _isactive = true;
}

void XH_Member::deactive(){
    _isactive = false;
}

void XH_Member::set_r(mpz_class& r){
    _r = r;
}

void XH_Member::set_x(mpz_class& x){
    _x = x;
}

void XH_Member::set_y(mpz_class& y){
    _y = y;
}

XH_Member::~XH_Member(){}