#ifndef _MYLIB_H
#define _MYLIB_H
#include <iostream>
#include <vector>
#include <gmpxx.h>

//CRT算法     返回 0 表示不是一致的，没有解
mpz_class CRT(std::vector<mpz_class>& remainders, std::vector<mpz_class>& modulus);

//获取解密密钥
mpz_class get_deckey(mpz_class& enc_key, mpz_class& modulus);

//获取主加密密钥
mpz_class get_master_key(std::vector<mpz_class>& enc_keys, std::vector<mpz_class>& modulus);

//PH加密
mpz_class PH_ENC(mpz_class& message, mpz_class& master_key, std::vector<mpz_class>& members_modulus);

//PH解密
mpz_class PH_DEC(mpz_class& ciphertext, mpz_class& private_key, mpz_class& modulus);


#endif