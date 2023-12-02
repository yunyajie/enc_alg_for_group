#include <iostream>
#include <gmpxx.h>
#include "mylib.h"
#include <vector>

int main(){
    std::vector<mpz_class> re = {5, 7, 9};
    std::vector<mpz_class> mod = {7, 11, 23};
    std::vector<mpz_class> private_keys;
    std::vector<mpz_class> message = {1, 2, 3, 4, 5, 6, 7, 10, 22, 24}; //加密的明文必须都小于最小的模数7
    for(int i = 0; i < re.size(); i++){
        mpz_class pk = get_deckey(re[i], mod[i]);
        private_keys.push_back(pk);
        std::cout << "enc_key = " << re[i] << " dec_key = " << pk << std::endl;
    }
    mpz_class m_key = get_master_key(re, mod);
    std::cout << "master key = " << m_key << std::endl;
    mpz_class c;
    for(int i = 0; i < message.size(); i++){
        std::cout << "加密消息 m = " << message[i] << ", 密文 c = " << (c = PH_ENC(message[i], m_key, mod)) << std::endl;
        std::cout << "所有成员解密结果如下：" << std::endl;
        for(int j = 0; j < private_keys.size(); j++){
            std::cout << "成员 " << j << " : 解密结果为 m' = " << PH_DEC(c, private_keys[j], mod[j]) << std::endl;
        }
    }

    return 0;
}