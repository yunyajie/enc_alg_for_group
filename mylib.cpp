#include "mylib.h"

//CRT算法     返回 0 表示不是一致的，没有解
mpz_class CRT(std::vector<mpz_class>& remainders, std::vector<mpz_class>& modulus){
    if((remainders.size() != modulus.size()) || remainders.empty()) return 0;
    mpz_class result(0);
    mpz_class m(1);
    int n = modulus.size();
    std::vector<mpz_class> x(n);
    //首先计算 M
    for(int i = 0; i < n; i++){
        m *= modulus[i];
    }
    //计算 x_i 的模 m_i 的逆 y_i
    std::vector<mpz_class> y(n);
    for(int i = 0; i < n; i++){
        x[i] = m / modulus[i];
        int success = mpz_invert(y[i].get_mpz_t(), x[i].get_mpz_t(), modulus[i].get_mpz_t());
        if(success == 0) return 0;
    }
    for(int i = 0; i < n; i++){
        result += remainders[i] * x[i] * y[i];
    }
    result %= m;
    return result;
}

//获取解密密钥
mpz_class get_deckey(mpz_class& enc_key, mpz_class& modulus){
    mpz_class dec_key;
    mpz_invert(dec_key.get_mpz_t(), enc_key.get_mpz_t(), static_cast<mpz_class>(modulus - 1).get_mpz_t());
    return dec_key;
}

//获取主加密密钥
mpz_class get_master_key(std::vector<mpz_class>& enc_keys, std::vector<mpz_class>& modulus){
    int n = modulus.size();
    mpz_class m(1);
    for(int i = 0; i < n; i++){
        m *= (modulus[i] - 1) / 2;
    }
    m *= 2;
    std::vector<mpz_class> x(n), y(n);
    for(int i = 0; i < n; i++){
        x[i] = m / (modulus[i] - 1);
        mpz_invert(y[i].get_mpz_t(), x[i].get_mpz_t(), static_cast<mpz_class>(modulus[i] - 1).get_mpz_t());
    }
    mpz_class m_key(0);
    for(int i = 0; i < n; i++){
        m_key += enc_keys[i] * x[i] * y[i];
    }
    m_key %= m;
    return m_key;
}

//PH加密
mpz_class PH_ENC(mpz_class& message, mpz_class& master_key, std::vector<mpz_class>& members_modulus){
    int n = members_modulus.size();
    mpz_class m(1);
    for(int i = 0; i < n; i++){
        m *= members_modulus[i];
    }
    mpz_class cipher_text;
    mpz_powm(cipher_text.get_mpz_t(), message.get_mpz_t(), master_key.get_mpz_t(), m.get_mpz_t());
    return cipher_text;
}

//PH解密
mpz_class PH_DEC(mpz_class& ciphertext, mpz_class& private_key, mpz_class& modulus){
    mpz_class message;
    mpz_powm(message.get_mpz_t(), ciphertext.get_mpz_t(), private_key.get_mpz_t(), modulus.get_mpz_t());
    return message;
}