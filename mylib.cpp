#include "mylib.h"

//CRT算法     返回 0 表示不是一致的，没有解
mpz_class CRT(const std::vector<mpz_class>& remainders, const std::vector<mpz_class>& modulus){
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

//获取一个随机的解密密钥 其值 < upperLimit，满足条件是 e = 2 * r + 1，即一个奇数，且小于其对应的模数
mpz_class get_enckey(const mpz_class& modulus){
    gmp_randstate_t state;
    //初始化 GMP 伪随机数生成器状态
    gmp_randinit_default(state);
    mpz_class upper = modulus - 1;
    mpz_class randomNum;
    //生成小于上限的随机数
    while(true){
        mpz_urandomm(randomNum.get_mpz_t(), state, upper.get_mpz_t());
        if(randomNum % 2 != 0 && randomNum != 1){
            return randomNum;
        }
    }
}

//获取解密密钥
mpz_class get_deckey(const mpz_class& enc_key, const mpz_class& modulus){
    mpz_class dec_key;
    mpz_invert(dec_key.get_mpz_t(), enc_key.get_mpz_t(), static_cast<mpz_class>(modulus - 1).get_mpz_t());
    return dec_key;
}

//系统初始化时获取主加密密钥
mpz_class get_master_key_init(const std::vector<mpz_class>& enc_keys, const std::vector<mpz_class>& modulus){
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

//初始化时获取主加密密钥   开放给外部的接口
mpz_class PH_MK_INIT(const std::vector<mpz_class>& enc_keys, const std::vector<mpz_class>& modulus){
    return get_master_key_init(enc_keys, modulus);
}

//成员变化后主加密密钥更新 flag = 1 表示成员增加   flag = 0 表示成员减少
void get_new_mk(mpz_class& m_key, const int flag, const std::vector<mpz_class>& chg_enc_keys, const std::vector<mpz_class>& chg_modulus, const mpz_class& parti_mul_product){
    if(chg_enc_keys.empty()) return;
    mpz_class temp(0);
    for(int i = 0; i < chg_enc_keys.size(); i++){
        temp += chg_enc_keys[i] * chg_modulus[i];
    }
    if(flag == 1){
        //成员加入
        m_key = (m_key + temp) % parti_mul_product;
    }else{
        //成员离开
        m_key = (m_key - temp) % parti_mul_product;
    }
}

//成员变化后主加密密钥更新 开放给外部的接口
void PH_MK_UPDATE(mpz_class& m_key, const int flag, const std::vector<mpz_class>& chg_enc_keys, const std::vector<mpz_class>& chg_modulus, const mpz_class& parti_mul_product){
    get_new_mk(m_key, flag, chg_enc_keys, chg_modulus, parti_mul_product);
}

//PH加密
mpz_class PH_ENC(const mpz_class& message, const mpz_class& master_key, const std::vector<mpz_class>& members_modulus){
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
mpz_class PH_DEC(const mpz_class& ciphertext, const mpz_class& private_key, const mpz_class& modulus){
    mpz_class message;
    mpz_powm(message.get_mpz_t(), ciphertext.get_mpz_t(), private_key.get_mpz_t(), modulus.get_mpz_t());
    return message;
}

//生成一个长度为 bit_length 的安全素数
void generate_safe_prime(mpz_class& safe_prime, const unsigned long int bit_length, const unsigned long int reps){
    mpz_class prime_candidate;
    gmp_randstate_t state;
    //初始化 GMP 伪随机数生成器状态
    gmp_randinit_default(state);
    //首先生成一个 bit_length 位的随机奇数
    mpz_urandomb(prime_candidate.get_mpz_t(), state, bit_length);
    mpz_setbit(prime_candidate.get_mpz_t(), bit_length - 2); //确保是一个 bit_length - 1 位长的数
    mpz_setbit(prime_candidate.get_mpz_t(), 0); //确保是一个奇数
    while(true){
        mpz_nextprime(safe_prime.get_mpz_t(), prime_candidate.get_mpz_t());
        //std::cout << "待选素数：" << safe_prime << std::endl;
        if(mpz_probab_prime_p(static_cast<mpz_class>((safe_prime - 1) / 2).get_mpz_t(), reps) == 2) return;
        prime_candidate = safe_prime;
    }
    
}

//生成一个大于 lowerLimit 的安全素数
void generate_safe_prime(mpz_class& safe_prime, const mpz_class& lowerLimit, const unsigned long int reps){
    mpz_class prime_candidate = lowerLimit;
    while(true){
        mpz_nextprime(safe_prime.get_mpz_t(), prime_candidate.get_mpz_t());
        //std::cout << "待选素数：" << safe_prime << std::endl;
        if(mpz_probab_prime_p(static_cast<mpz_class>((safe_prime - 1) / 2).get_mpz_t(), reps) == 2) return;
        prime_candidate = safe_prime;
    }
}