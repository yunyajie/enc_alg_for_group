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
mpz_class get_enckey(const mpz_class& modulus, const mpz_class& upperLimit){
    gmp_randstate_t state;
    //初始化 GMP 伪随机数生成器状态
    gmp_randinit_default(state);
    mpz_class upper = upperLimit - 1;
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

//获取主加密密钥
mpz_class get_master_key(const std::vector<mpz_class>& enc_keys, const std::vector<mpz_class>& modulus){
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

//素性测试 reps 是 Miller_Rabin 素性测试的重复次数
bool is_prime(const mpz_class& num, const unsigned long int reps){
    //使用 Miller_Rabin 素性测试进行检查
    return mpz_probab_prime_p(num.get_mpz_t(), reps);
}

//生成一个长度为 bit_length 的安全素数
void generate_safe_prime(mpz_class& safe_prime, const unsigned long int bit_length, const unsigned long int reps){
    mpz_class prime_candidate;
    gmp_randstate_t state;
    //初始化 GMP 伪随机数生成器状态
    gmp_randinit_default(state);
    //首先生成一个 bit_length 位的随机奇数
    mpz_urandomb(prime_candidate.get_mpz_t(), state, bit_length);
    mpz_setbit(prime_candidate.get_mpz_t(), bit_length - 1); //确保是一个 bit_length 位长的数
    mpz_setbit(prime_candidate.get_mpz_t(), 0); //确保是一个奇数

    std::cout << "这个随机的奇数是：" << prime_candidate << std::endl;
    while(!is_prime(prime_candidate, reps) || !is_prime((prime_candidate - 1) / 2, reps)){
        //不是素数就试探下一个奇数
        mpz_add_ui(prime_candidate.get_mpz_t(), prime_candidate.get_mpz_t(), 2);
    }
}