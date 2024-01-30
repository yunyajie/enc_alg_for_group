#ifndef _UTILITY_H_
#define _UTILITY_H_
#include<gmpxx.h>
#include<string>
#include<vector>
#include<openssl/sha.h>
using namespace std;
//工具

//将 strs 中的字符串用 SHA256 做哈希并转换成为 mpz_class 格式存入 result 中
int sha256encrypt(vector<string>& strs, mpz_class& result);


#endif