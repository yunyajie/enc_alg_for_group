#include "XH_Member.h"
XH_Member::XH_Member(mpz_class& modulus):_m(modulus){
}

mpz_class XH_Member::decrypt(const mpz_class& ciphertext){
    mpz_class r = ciphertext % _m;   //先获取余数
    std::vector<string> strs = {_m.get_str(), _server_r.get_str()};
    mpz_class md;
    sha256encrypt(strs, md);
    mpz_class gk = r ^ md;
    return gk;
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

void XH_Member::set_server_r(mpz_class& server_r){
    _server_r = server_r;
}

void XH_Member::set_x(mpz_class& x){
    _x = x;
}

void XH_Member::set_y(mpz_class& y){
    _y = y;
}

XH_Member::~XH_Member(){}