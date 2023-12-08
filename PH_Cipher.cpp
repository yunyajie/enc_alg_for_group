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

mpz_class PH_Member::get_enc_key() const{
    return enc_key;
}

mpz_class PH_Member::get_dec_key() const{
    return dec_key;
}

mpz_class PH_Member::get_modulus() const{
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

//新成员注册，系统分配可用密钥
int PH_Cipher::allocation(PH_Member& new_register){
    if(this->available < m){ //还有可用的密钥
        new_register = this->members[available];
    }else{ //available  == m 密钥分配完毕，需要扩展系统
        sys_entend();
        new_register = this->members[available];
    }
    this->available++;
}


mpz_class PH_Cipher::encrypt(const mpz_class& message){
    mpz_class cipher_text;
    mpz_powm(cipher_text.get_mpz_t(), message.get_mpz_t(), m_key.get_mpz_t(), active_mod_product.get_mpz_t());
    return cipher_text;
}

int PH_Cipher::member_join(const PH_Member& joiner){
    this->active_mod_product *= joiner.get_modulus();
    this->active_lcm *= (joiner.get_modulus() - 1) / 2;
    return 0;
}

int PH_Cipher::member_leave(const PH_Member& leaver){
    
    return 0;
}



//系统扩展为原来的两倍
int PH_Cipher::sys_entend(){
    std::vector<PH_Member> newmembers = init_members(m);
    return 0;
}

PH_Cipher::~PH_Cipher(){}



//系统初始化
void PH_Cipher::sys_init(){
    //使用 bit_length 初始化 modulus_lower_bound
    gmp_randstate_t state;
    gmp_randinit_default(state);    //初始化 GMP 伪随机数生成器状态
    mpz_urandomb(modulus_lower_bound.get_mpz_t(), state, bit_length);   //首先生成一个 bit_length 位的随机奇数
    mpz_setbit(modulus_lower_bound.get_mpz_t(), bit_length - 1); //确保是一个 bit_length 位长的数
    mpz_setbit(modulus_lower_bound.get_mpz_t(), 0); //确保是一个奇数

    //初始化 m 个成员
    this->members.clear();
    this->members = init_members(m);

    //初始化 mod_product 和 lcm
    this->mod_product = 1;
    this->lcm = 1;
    for(auto ele : this->members){
        this->lcm *= static_cast<mpz_class>((ele.get_modulus() - 1) / 2);
        this->mod_product *= ele.get_modulus();
    }
    this->lcm *= 2;

    //初始化 x 和 y
    this->x.clear();
    this->y.clear();
    mpz_class tem_x, tem_y;
    mpz_class lcm_half = this->lcm / 2;
    for(auto ele : this->members){
        tem_x = lcm_half / static_cast<mpz_class>((ele.get_modulus() - 1) / 2);
        mpz_invert(tem_y.get_mpz_t(), tem_x.get_mpz_t(), static_cast<mpz_class>((ele.get_modulus() - 1) / 2).get_mpz_t());
        this->x.push_back(tem_x);
        this->y.push_back(tem_y);
    }

    //初始化 available 从下标为 1 的位置开始分配，将 m_key 初始化为第一个 e_i * x_i * y_i
    this->available = 1;

    //初始化 m_key
    master_key_init();

    //初始化 active_mod_product active_lcm
    this->active_mod_product = 1;  //未有成员注册并加入活跃组
    this->active_lcm = 1;

}

//生成一个大于 lowerLimit 的安全素数
void PH_Cipher::generate_safe_prime(mpz_class& safe_prime, const unsigned long int reps){
    mpz_class prime_candidate = this->modulus_lower_bound;
    while(true){
        mpz_nextprime(safe_prime.get_mpz_t(), prime_candidate.get_mpz_t());
        if(mpz_probab_prime_p(static_cast<mpz_class>((safe_prime - 1) / 2).get_mpz_t(), reps) == 2){
            //更新安全素数下界
            this->modulus_lower_bound = safe_prime;
            return;
        }
        prime_candidate = safe_prime + 1;
    }
}

//获取一个随机的解密密钥 其值 < upperLimit，满足条件是 e = 2 * r + 1，即一个奇数，且小于其对应的模数
mpz_class get_enckey(const mpz_class& modulus){
    // 使用当前时间的微秒级别信息作为种子
    auto seed = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    gmp_randstate_t state;
    //初始化 GMP 伪随机数生成器状态
    gmp_randinit_default(state);
    gmp_randseed_ui(state, static_cast<unsigned long>(seed));
    mpz_class upper = modulus - 1;
    mpz_class randomNum;
    //生成小于上限的随机数
    mpz_urandomm(randomNum.get_mpz_t(), state, upper.get_mpz_t());
    //将随机数置为奇数
    randomNum |= 1;
    //清理状态
    gmp_randclear(state);
    return randomNum;
}


std::vector<PH_Member> PH_Cipher::init_members(int n){
    std::vector<PH_Member> new_members;
    mpz_class mod;
    mpz_class enc_key;
    for(int i = 0; i < n; i++){
        generate_safe_prime(mod, 50);
        enc_key = get_enckey(mod);
        new_members.emplace_back(enc_key, mod);
    }
    return new_members;
}

//系统初始化后初始化 m_key
void PH_Cipher::master_key_init(){
    this->m_key = 0;
    for(int i = 0; i < this->members.size(); i++){
        this->m_key += this->members[i].get_enc_key() * x[i] * y[i];
    }
    this->m_key %= this->lcm;

    /*
    设 mod_i = 2 * p_i + 1,
    则
    m_key % p_i = e_i,
    又 m_key % 2 = 1, e_i % 2 = 1,  e_i < 2 * p_i
    有 m_key % (2 * p_i) = e_i
    */
    //需要满足 m_key % 2 == 1
    if((this->m_key & 1) == 0) this->m_key = (this->m_key + this->lcm / 2) % (this->lcm);

    //std::cout << "master_key = " << this->m_key << std::endl;
}


//测试用   此时假设所有成员都是活跃成员
void PH_Cipher::test(){
    this->active_mod_product = this->mod_product;
    std::vector<int> messages = {111234567, 12123453, 8123452, 912989, 111231, 93330, 11134512};
    int n = messages.size();
    std::cout << "*****************************加密消息测试*****************************" << std::endl;
    mpz_class c;
    for(int i = 0; i < n; i++){
        std::cout << "***********************************************************************" <<std::endl;
        std::cout << "消息" << i << ": " << messages[i] << "     加密后密文：c = " << (c = encrypt(messages[i])) << std::endl;
        std::cout << "解密情况如下：" << std::endl;
        for(int j = 0; j < m; j++){
            std::cout << "成员 " << j << " : 密钥为 (" << this->members[j].get_dec_key() << "," << this->members[j].get_modulus() << ")   " 
            << "解密的消息为：" << this->members[j].decrypt(c) << std::endl;
        }
    }
}