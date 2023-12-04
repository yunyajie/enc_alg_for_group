#include <iostream>
#include <gmpxx.h>
#include "mylib.h"
#include <vector>

int main(){
    std::vector<mpz_class> mod;
    std::vector<mpz_class> enc_keys;
    mpz_class m_key;
    std::vector<mpz_class> dec_keys;
    std::vector<mpz_class> messages = {2, 4, 18, 123, 378, 267, 190, 89, 78, 14, 23, 47, 88, 89};
    //生成 10 个 1024 位的安全素数
    mpz_class safe_prime;
    mpz_class enc_k, dec_k;
    generate_safe_prime(safe_prime, 10, 25);
    for(int i = 1; i < 10; i++){
        std::cout << "第 " << i << " 个 10 位的安全素数 ：" << safe_prime << std::endl;
        mod.push_back(safe_prime);
        enc_k = get_enckey(safe_prime);
        enc_keys.push_back(enc_k);
        dec_k = get_deckey(enc_k, safe_prime);
        dec_keys.push_back(dec_k);
        std::cout << "enc_key = " << enc_k << std::endl << "dec_key = " << dec_k << std::endl;
        generate_safe_prime(safe_prime, safe_prime, 25);
    }
    m_key = PH_MK_INIT(enc_keys, mod);
    std::cout << std::endl << std::endl << "主加密密钥为 m_key = " << m_key << std::endl;
    mpz_class c;
    for(int i = 0; i < messages.size(); i++){
        std::cout << "消息 m = " << messages[i] << "  ----> 密文 c = " << (c = PH_ENC(messages[i], m_key, mod)) 
        << std::endl;
        std::cout << "所有成员解密情况如下：" << std::endl;
        for(int j = 0; j < mod.size(); j++){
            std::cout << "成员 " << j << ": " << PH_DEC(c, dec_keys[j], mod[j]) << std::endl;
        }
    }
    return 0;
}