#include "PH_Cipher.h"

//PH_Member 的实现

PH_Member::PH_Member(mpz_class enc_key, mpz_class modulus):enc_key(enc_key), modulus(modulus){
    //初始化解密密钥
    mpz_invert(dec_key.get_mpz_t(), enc_key.get_mpz_t(), static_cast<mpz_class>(modulus - 1).get_mpz_t());
}

mpz_class PH_Member::decrypt(const mpz_class& ciphertext){
    mpz_class message;
    mpz_powm(message.get_mpz_t(), ciphertext.get_mpz_t(), dec_key.get_mpz_t(), modulus.get_mpz_t());
    return message;
}

mpz_class PH_Member::get_enc_key(){
    return enc_key;
}

mpz_class PH_Member::get_dec_key(){
    return dec_key;
}

mpz_class PH_Member::get_modulus(){
    return modulus;
}

PH_Member::~PH_Member(){}





//PH_Cipher 的实现

PH_Cipher::PH_Cipher(int m, int bit_length):m(m), bit_length(bit_length){
}

//其他初始化的内容全部放在这里
void PH_Cipher::init(){
    //系统初始化
    sys_init();
}

mpz_class PH_Cipher::encrypt(const mpz_class& message){
    mpz_class cipher_text;
    mpz_powm(cipher_text.get_mpz_t(), message.get_mpz_t(), m_key.get_mpz_t(), active_mod_product.get_mpz_t());
    return cipher_text;
}

int PH_Cipher::member_join(const PH_Member joiner){

    return 0;
}

int PH_Cipher::member_leave(const PH_Member leaver){
    
    return 0;
}



//系统扩展为原来的两倍
int PH_Cipher::sys_entend(){
    std::vector<PH_Member> newmembers = init_members(m);

}

PH_Cipher::~PH_Cipher(){}



//系统初始化
void PH_Cipher::sys_init(){
    //使用 bit_length 初始化 modulus_lower_bound
    gmp_randstate_t state;
    gmp_randinit_default(state);    //初始化 GMP 伪随机数生成器状态
    mpz_urandomb(modulus_lower_bound.get_mpz_t(), state, bit_length);   //首先生成一个 bit_length 位的随机奇数
    mpz_setbit(modulus_lower_bound.get_mpz_t(), bit_length - 2); //确保是一个 bit_length - 1 位长的数
    mpz_setbit(modulus_lower_bound.get_mpz_t(), 0); //确保是一个奇数

    //初始化 m 个成员
    members = init_members(m);

    //初始化 mod_product 和 lcm
    mod_product = 1;
    lcm = 1;
    for(int i = 0; i < m; i++){
        lcm *= (members[i].get_modulus() - 1) / 2;
        mod_product *= members[i].get_modulus();
    }
    lcm *= 2;
    
    //初始化 x 和 y
    mpz_class tem_x, tem_y;
    for(int i = 0; i < m; i++){
        tem_x = lcm / (members[i].get_modulus() - 1);
        mpz_invert(tem_y.get_mpz_t(), tem_x.get_mpz_t(), static_cast<mpz_class>(members[i].get_modulus() - 1).get_mpz_t());
        x.push_back(tem_x);
        y.push_back(tem_y);
    }

    //初始化 available
    available = 0;

    //初始化 m_key
    master_key_init();

    //初始化 active_mod_product
    active_mod_product = 1;  //未有成员注册并加入活跃组

}

//生成一个大于 lowerLimit 的安全素数
void PH_Cipher::generate_safe_prime(mpz_class& safe_prime, const unsigned long int reps){
    mpz_class prime_candidate = modulus_lower_bound;
    while(true){
        mpz_nextprime(safe_prime.get_mpz_t(), prime_candidate.get_mpz_t());
        //std::cout << "待选素数：" << safe_prime << std::endl;
        if(mpz_probab_prime_p(static_cast<mpz_class>((safe_prime - 1) / 2).get_mpz_t(), reps) == 2){
            //更新安全素数下界
            modulus_lower_bound = safe_prime;
            return;
        }
        prime_candidate = safe_prime;
    }
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


std::vector<PH_Member> PH_Cipher::init_members(int n){
    std::vector<PH_Member> members;
    mpz_class mod;
    mpz_class dec_key;
    mpz_class enc_key;
    for(int i = 0; i < n; i++){
        generate_safe_prime(mod, 25);
        enc_key = get_enckey(mod);
        members.emplace_back(enc_key, mod);
    }
    return members;
}

//系统初始化后初始化 m_key
void PH_Cipher::master_key_init(){
    m_key = 0;
    for(int i = 0; i < m; i++){
        m_key += members[i].get_enc_key() * x[i] * y[i];
    }
    m_key %= lcm;
}


//测试用   此时假设所有成员都是活跃成员
void PH_Cipher::test(){
    active_mod_product = mod_product;
    std::vector<int> messages = {111111, 12356787, 82138, 99918, 111234, 90008, 112344};
    int n = messages.size();
    std::cout << "*****************************加密消息测试*****************************" << std::endl;
    for(int i = 0; i < n; i++){
        std::cout << "***********************************************************************" <<std::endl;
        std::cout << "消息" << i << ": " << messages[i] << "     加密后密文：c = " << encrypt(messages[i]) << std::endl;
        std::cout << "解密情况如下：" << std::endl;
        for(int j = 0; j < m; j++){
            std::cout << "成员 " << j << " : 密钥为 (" << members[j].get_dec_key() << "," << members[j].get_modulus() << ")   " 
            << "解密的消息为：" << members[j].decrypt(messages[i]) << std::endl;
        }
    }
}