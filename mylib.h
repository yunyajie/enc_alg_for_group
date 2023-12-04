#ifndef _MYLIB_H
#define _MYLIB_H
#include <iostream>
#include <vector>
#include <gmpxx.h>

//CRT算法     返回 0 表示不是一致的，没有解
mpz_class CRT(const std::vector<mpz_class>& remainders, const std::vector<mpz_class>& modulus);

//获取一个随机的解密密钥 其值 < upperLimit，满足条件是 e = 2 * r + 1，即一个奇数，且小于其对应的模数
mpz_class get_enckey(const mpz_class& modulus);

//获取解密密钥
mpz_class get_deckey(const mpz_class& enc_key, const mpz_class& modulus);

//系统初始化时获取主加密密钥   开放给外部的接口
mpz_class PH_MK_INIT(const std::vector<mpz_class>& enc_keys, const std::vector<mpz_class>& modulus);

//PH加密
mpz_class PH_ENC(const mpz_class& message, const mpz_class& master_key, const std::vector<mpz_class>& members_modulus);

//PH解密
mpz_class PH_DEC(const mpz_class& ciphertext, const mpz_class& private_key, const mpz_class& modulus);

//生成一个长度为 bit_length 的安全素数
void generate_safe_prime(mpz_class& safe_prime, const unsigned long int bit_length, const unsigned long int reps);

//生成一个大于 lowerLimit 的安全素数
void generate_safe_prime(mpz_class& safe_prime, const mpz_class& lowerLimit, const unsigned long int reps);


#endif