#include "PH_Cipher.h"

//PH_Member 的实现
PH_Member::PH_Member(){
    //空构造
}

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
    //系统初始化
    sys_init();
}

//新成员注册，系统分配可用密钥
int PH_Cipher::allocation(PH_Member& new_register){
    if(this->available < this->m){ //还有可用的密钥
        new_register = this->members[this->available];
    }else{ //available == m 密钥分配完毕，需要扩展系统
        sys_entend();
        new_register = this->members[this->available];
    }
    this->available++;
    return 0;
}


mpz_class PH_Cipher::encrypt(const mpz_class& message){
    mpz_class cipher_text;
    mpz_powm(cipher_text.get_mpz_t(), message.get_mpz_t(), m_key.get_mpz_t(), active_mod_product.get_mpz_t());
    return cipher_text;
}

int PH_Cipher::member_join(const PH_Member& joiner){
    if(this->mod_map.find(joiner.get_modulus()) == this->mod_map.end()) return -1;
    //首先获取 leaver 在 系统成员列表中的下标
    int index = this->mod_map[joiner.get_modulus()];
    //首先判断这个成员是否在活跃组中
    if(find(this->active_members_index.begin(), this->active_members_index.end(), index) != this->active_members_index.end()){
        //在活跃组中
        //std::cout << "成员在活跃组中，不可重复加入！！！！" << std::endl;
        return -1;
    }
    this->active_mod_product *= joiner.get_modulus();
    this->active_lcm *= (joiner.get_modulus() - 1) / 2;
    //更新 active_members_index
    
    //std::cout << "index = " << index << std::endl;
    this->active_members_index.push_back(index);
    //更新主加密密钥
    this->m_key += this->members[index].get_enc_key() * this->x[index] * this->y[index];
    this->m_key %= this->lcm;
    if((this->m_key & 1) == 0) this->m_key = (this->m_key + this->lcm / 2) % this->lcm;
    std::cout << "主密钥更新  master_key = " << this->m_key << std::endl;
    return 0;
}

int PH_Cipher::member_leave(const PH_Member& leaver){
    if(this->mod_map.find(leaver.get_modulus()) == this->mod_map.end()) return -1;
    //首先获取 leaver 在 系统成员列表中的下标
    int index = this->mod_map[leaver.get_modulus()];
    //首先判断这个成员是否在活跃组中
    if(find(this->active_members_index.begin(), this->active_members_index.end(), index) == this->active_members_index.end()){
        //不在活跃组中
        //std::cout << "成员不在活跃组中，无需退出！！！！" << std::endl;
        return -1;
    }
    this->active_mod_product /= leaver.get_modulus();
    this->active_lcm /= (leaver.get_modulus() - 1) / 2;
    //更新 active_members_index
    this->active_members_index.erase(std::remove(this->active_members_index.begin(), this->active_members_index.end(), index));
    //更新主加密密钥
    this->m_key -= this->members[index].get_enc_key() * this->x[index] * this->y[index];
    //mpz_mod(this->m_key.get_mpz_t(), this->m_key.get_mpz_t(), this->lcm.get_mpz_t());   //保证 m_key 是正数
    this->m_key = (this->m_key % this->lcm + this->lcm) % this->lcm;  //保证 m_key 是正数
    if((this->m_key & 1) == 0) this->m_key = (this->m_key + this->lcm / 2) % this->lcm;
    std::cout << "主密钥更新  master_key = " << this->m_key << std::endl;
    return 0;
}

//系统扩展为原来的两倍
int PH_Cipher::sys_entend(){
    std::cout << ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>系统拓展<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<" << std::endl;
    std::vector<PH_Member> newmembers = init_members(this->m);
    //将新的密钥加入到已有密钥的尾部
    this->members.insert(this->members.end(), newmembers.begin(), newmembers.end());
    //更新 mod_map
    int cnt = this->m;
    for(PH_Member ele : newmembers){
        mod_map[ele.get_modulus()] = cnt++;
    }
    //更新 m
    this->m *= 2;
    //初始化 mod_product 和 lcm
    init_lcm_modproduct();
    //初始化 x 和 y
    init_xy();
    //初始化 m_key
    master_key_init();
    return 0;
}

PH_Cipher::~PH_Cipher(){}

//初始化 x 和 y
void PH_Cipher::init_xy(){
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
}

//初始化 lcm 和 mod_product
void PH_Cipher::init_lcm_modproduct(){
    this->mod_product = 1;
    this->lcm = 1;
    for(auto ele : this->members){
        this->lcm *= static_cast<mpz_class>((ele.get_modulus() - 1) / 2);
        this->mod_product *= ele.get_modulus();
    }
    this->lcm *= 2;
}

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
    int cnt = 0;
    //初始化 mod_map
    for(PH_Member ele : this->members){
        this->mod_map[ele.get_modulus()] = cnt++;
    }

    //初始化 mod_product 和 lcm
    init_lcm_modproduct();

    //初始化 x 和 y
    init_xy();

    //初始化 available 从下标为 0 的位置开始分配
    this->available = 0;

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
    //根据 active_members 中的活跃成员初始化 m_key
    this->m_key = 0;
    if(this->active_members_index.empty()) return;  //活跃组还未有人，初始化为 0
    for(int ele : this->active_members_index){
        this->m_key += this->members[ele].get_enc_key() * this->x[ele] * this->y[ele];
    }
    this->m_key %= this->lcm;

    /*
    e_i = 2 * r_i + 1 是一个奇数
    设 mod_i = 2 * p_i + 1, 下面的代码保证 m_key mod 2 = 1，是一个奇数
    则整理所有条件有： 
    m_key mod 2 = 1                (1)  
    m_key mod p_i = e_i mod p_i    (2)
    于是有：
    由 (2) m_key = k * p_i + e_i
    由 m_key 和 p_i 都是奇数， k * p_i 是偶数，又有 p_i 是素数，k 是偶数，于是 (k * p_i) mod (2 * p_i) = 0
    结论：
    m_key mod (2 * p_i) 
    = (k * p_i + e_i) mod (2 * p_i) 
    = (k * p_i) mod (2 * p_i) + e_i mod (2 * p_i) 
    = e_i mod (2 * p_i) 
    = e_i
    */
    //需要满足 m_key % 2 == 1
    if((this->m_key & 1) == 0) this->m_key = (this->m_key + this->lcm / 2) % this->lcm;

    std::cout << "主密钥更新  master_key = " << this->m_key << std::endl;
    // for(int i = 0; i < this->members.size(); i++){
    //     std::cout << "u_" << i << " : x = " << (members[i].get_modulus() - 1) / 2 << "   e = " << members[i].get_enc_key() << "   m_key mod (2 * x) = " << m_key % (2 * ((members[i].get_modulus() - 1) / 2)) << std::endl;
    // }
}