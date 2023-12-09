#include <iostream>
#include <gmpxx.h>
#include "PH_Cipher.h"
#include <vector>

int main(){
    //初始化一个容量为 2 的系统
    PH_Cipher ph_cipher(2, 32);
    ph_cipher.init();
    std::vector<PH_Member> test_m;
    //假设先后一共有 3 位成员加入，有一些成员其间加入退出  系统会拓展
    test_m = std::vector<PH_Member>(3);
    //系统依次给前两个人分配密钥使他们成为系统成员
    std::cout <<"前两个成员注册：" << std::endl;
    ph_cipher.allocation(test_m[0]);
    ph_cipher.allocation(test_m[1]);
    //第一个成员加入活跃组
    std::cout << std::endl <<"第一个成员加入：" << std::endl;
    ph_cipher.member_join(test_m[0]);

    //加解密测试
    std::vector<int> messages = {111234567, 12123453, 8123452};
    int n = messages.size();
    std::cout << "*****************************加密消息测试*****************************" << std::endl;
    mpz_class c;
    for(int i = 0; i < n; i++){
        std::cout << "***********************************************************************" <<std::endl;
        std::cout << "消息" << i << ": " << messages[i] << "     加密后密文：c = " << (c = ph_cipher.encrypt(messages[i])) << std::endl;
        std::cout << "解密情况如下：" << std::endl;
        for(int j = 0; j < 2; j++){
            std::cout << "成员 " << j << " : 密钥为 (" << test_m[j].get_dec_key() << "," << test_m[j].get_modulus() << ")   " 
            << "解密的消息为：" << test_m[j].decrypt(c) << std::endl;
        }
    }

    //第二个成员加入
    std::cout << std::endl <<"第二个成员加入：" << std::endl;
    ph_cipher.member_join(test_m[1]);
    //加解密测试
    std::cout << "*****************************加密消息测试*****************************" << std::endl;
    for(int i = 0; i < n; i++){
        std::cout << "***********************************************************************" <<std::endl;
        std::cout << "消息" << i << ": " << messages[i] << "     加密后密文：c = " << (c = ph_cipher.encrypt(messages[i])) << std::endl;
        std::cout << "解密情况如下：" << std::endl;
        for(int j = 0; j < 2; j++){
            std::cout << "成员 " << j << " : 密钥为 (" << test_m[j].get_dec_key() << "," << test_m[j].get_modulus() << ")   " 
            << "解密的消息为：" << test_m[j].decrypt(c) << std::endl;
        }
    }

    //第一个成员退出
    std::cout << std::endl <<"第一个成员退出：" << std::endl;
    ph_cipher.member_leave(test_m[0]);
    //加解密测试
    std::cout << "*****************************加密消息测试*****************************" << std::endl;
    for(int i = 0; i < n; i++){
        std::cout << "***********************************************************************" <<std::endl;
        std::cout << "消息" << i << ": " << messages[i] << "     加密后密文：c = " << (c = ph_cipher.encrypt(messages[i])) << std::endl;
        std::cout << "解密情况如下：" << std::endl;
        for(int j = 0; j < 2; j++){
            std::cout << "成员 " << j << " : 密钥为 (" << test_m[j].get_dec_key() << "," << test_m[j].get_modulus() << ")   " 
            << "解密的消息为：" << test_m[j].decrypt(c) << std::endl;
        }
    }

    //第三个成员注册，系统会进行拓展
    std::cout << std::endl <<"第三个成员注册，系统会进行拓展：" << std::endl;
    ph_cipher.allocation(test_m[2]);
    //加解密测试
    std::cout << "*****************************加密消息测试*****************************" << std::endl;
    for(int i = 0; i < n; i++){
        std::cout << "***********************************************************************" <<std::endl;
        std::cout << "消息" << i << ": " << messages[i] << "     加密后密文：c = " << (c = ph_cipher.encrypt(messages[i])) << std::endl;
        std::cout << "解密情况如下：" << std::endl;
        for(int j = 0; j < 3; j++){
            std::cout << "成员 " << j << " : 密钥为 (" << test_m[j].get_dec_key() << "," << test_m[j].get_modulus() << ")   " 
            << "解密的消息为：" << test_m[j].decrypt(c) << std::endl;
        }
    }

    //第三个成员加入
    std::cout << std::endl <<"第三个成员加入：" << std::endl;
    ph_cipher.member_join(test_m[2]);
    //加解密测试
    std::cout << "*****************************加密消息测试*****************************" << std::endl;
    for(int i = 0; i < n; i++){
        std::cout << "***********************************************************************" <<std::endl;
        std::cout << "消息" << i << ": " << messages[i] << "     加密后密文：c = " << (c = ph_cipher.encrypt(messages[i])) << std::endl;
        std::cout << "解密情况如下：" << std::endl;
        for(int j = 0; j < 3; j++){
            std::cout << "成员 " << j << " : 密钥为 (" << test_m[j].get_dec_key() << "," << test_m[j].get_modulus() << ")   " 
            << "解密的消息为：" << test_m[j].decrypt(c) << std::endl;
        }
    }

    //第一个成员加入
    std::cout << std::endl <<"第一个成员加入：" << std::endl;
    ph_cipher.member_join(test_m[0]);
    //加解密测试
    std::cout << "*****************************加密消息测试*****************************" << std::endl;
    for(int i = 0; i < n; i++){
        std::cout << "***********************************************************************" <<std::endl;
        std::cout << "消息" << i << ": " << messages[i] << "     加密后密文：c = " << (c = ph_cipher.encrypt(messages[i])) << std::endl;
        std::cout << "解密情况如下：" << std::endl;
        for(int j = 0; j < 3; j++){
            std::cout << "成员 " << j << " : 密钥为 (" << test_m[j].get_dec_key() << "," << test_m[j].get_modulus() << ")   " 
            << "解密的消息为：" << test_m[j].decrypt(c) << std::endl;
        }
    }

    return 0;
}