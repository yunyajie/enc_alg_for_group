#include "PH_Cipher.h"

//PH_Member 的实现
PH_Member::PH_Member():isregistered(false), isactive(false){
    //空构造
}

PH_Member::PH_Member(mpz_class enc_key, mpz_class modulus):enc_key(enc_key), modulus(modulus), isregistered(false), isactive(false){
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

bool PH_Member::isRegistered() const{
    return isregistered;
}

bool PH_Member::isActive() const{
    return isactive;
}

void PH_Member::registered(){
    isregistered = true;
}

void PH_Member::active(){
    isactive = true;
}

void PH_Member::deactive(){
    isactive = false;
}

void PH_Member::set_x(mpz_class& x){
    this->x = x;
}

void PH_Member::set_y(mpz_class& y){
    this->y = y;
}

mpz_class PH_Member::get_x(){
    return this->x;
}

mpz_class PH_Member::get_y(){
    return this->y;
}

PH_Member::~PH_Member(){}





//PH_Cipher 的实现

PH_Cipher::PH_Cipher(int m, int bit_length):m(m), bit_length(bit_length){
    //系统初始化
    sys_init();
}

//新成员注册，系统分配可用密钥
int PH_Cipher::allocation(PH_Member& new_register){
    if(available.empty()){//密钥分配完毕，系统需要扩展
        sys_extend();
    }
    //分配密钥
    mpz_class t = *available.begin();
    available.erase(t);
    this->members[t].registered();
    new_register = this->members[t];
    return 0;
}


mpz_class PH_Cipher::encrypt(const mpz_class& message){
    mpz_class cipher_text;
    mpz_powm(cipher_text.get_mpz_t(), message.get_mpz_t(), m_key.get_mpz_t(), active_mod_product.get_mpz_t());
    return cipher_text;
}

int PH_Cipher::member_join(PH_Member& joiner){      //注意这里的 joiner 的 x 和 y 参数并没有更新
    if(members.find(joiner.get_modulus()) == members.end()) return -1; //成员不在系统中
    //首先判断这个成员是否在活跃组中
    if(active_members.find(joiner.get_modulus()) != active_members.end()){
        //在活跃组中，主密钥不需要更新，直接返回
        return 0;
    }
    //更新成员状态
    joiner.active();
    members[joiner.get_modulus()].active();
    //加入活跃组
    active_members.insert(joiner.get_modulus());
    //更新系统状态
    this->active_mod_product *= joiner.get_modulus();
    this->active_lcm *= (joiner.get_modulus() - 1) / 2;

    //更新主加密密钥
    PH_Member&t = members[joiner.get_modulus()];
    this->m_key += t.get_enc_key() * t.get_x() * t.get_y();
    this->m_key %= this->lcm;
    if((this->m_key & 1) == 0) this->m_key = (this->m_key + this->lcm / 2) % this->lcm;

    std::cout << "主密钥更新  master_key = " << this->m_key << std::endl;
    return 0;
}

int PH_Cipher::member_leave(PH_Member& leaver){     //注意这里的 leaver 的 x 和 y 参数并没有更新
    if(members.find(leaver.get_modulus()) == members.end()) return -1; //成员不在该系统中
    //首先判断这个成员是否在活跃组中
    if(active_members.find(leaver.get_modulus()) == active_members.end()){
        //成员不在活跃组中，直接返回
        return -1;
    }
    //成员在活跃组中，更新系统参数
    active_mod_product /= leaver.get_modulus();
    active_lcm /= (leaver.get_modulus() - 1) / 2;
    //从活跃组中删除
    active_members.erase(leaver.get_modulus());
    //更新成员状态
    leaver.deactive();
    members[leaver.get_modulus()].deactive();
    //更新主加密密钥
    PH_Member&t = members[leaver.get_modulus()];
    this->m_key -= t.get_enc_key() * t.get_x() * t.get_y();
    //mpz_mod(this->m_key.get_mpz_t(), this->m_key.get_mpz_t(), this->lcm.get_mpz_t());   //保证 m_key 是正数
    this->m_key = (this->m_key % this->lcm + this->lcm) % this->lcm;  //保证 m_key 是正数
    if((this->m_key & 1) == 0) this->m_key = (this->m_key + this->lcm / 2) % this->lcm;

    std::cout << "主密钥更新  master_key = " << this->m_key << std::endl;
    return 0;
}

int PH_Cipher::active_size(){
    return active_members.size();
}

PH_Cipher::~PH_Cipher(){}

//系统扩展为原来的两倍
int PH_Cipher::sys_extend(){
    std::cout << ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>系统拓展<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<" << std::endl;
    std::vector<PH_Member> newmembers = init_members(this->m);
    //将新的密钥加入到系统的成员集合中以及可分配集合中
    for(auto& ele : newmembers){
        members.insert(std::make_pair(ele.get_modulus(), ele));
        available.insert(ele.get_modulus());
    }
    //更新系统参数
    init_lcm_modproduct();
    init_xy();
    //更新 m
    this->m *= 2;
    //初始化 m_key
    master_key_init();
    return 0;
}

//初始化 x 和 y
void PH_Cipher::init_xy(){
    mpz_class tem_x, tem_y;
    mpz_class lcm_half = this->lcm / 2;
    for(auto& ele : this->members){
        tem_x = lcm_half / static_cast<mpz_class>((ele.first - 1) / 2);
        ele.second.set_x(tem_x);
        mpz_invert(tem_y.get_mpz_t(), tem_x.get_mpz_t(), static_cast<mpz_class>((ele.first - 1) / 2).get_mpz_t());
        ele.second.set_y(tem_y);
    }
}

//初始化 lcm 和 mod_product
void PH_Cipher::init_lcm_modproduct(){
    this->mod_product = 1;
    this->lcm = 1;
    for(auto& ele : this->members){
        mpz_class mod = ele.first;
        this->lcm *= static_cast<mpz_class>((mod - 1) / 2);
        this->mod_product *= mod;
    }
    this->lcm *= 2;
}

//系统初始化
void PH_Cipher::sys_init(){
    //使用 bit_length 初始化 modulus_lower_bound 如果是从数据库载入，则取其中最大的模数为下界
    gmp_randstate_t state;
    gmp_randinit_default(state);    //初始化 GMP 伪随机数生成器状态
    mpz_urandomb(modulus_lower_bound.get_mpz_t(), state, bit_length);   //首先生成一个 bit_length 位的随机奇数
    mpz_setbit(modulus_lower_bound.get_mpz_t(), bit_length - 1); //确保是一个 bit_length 位长的数
    mpz_setbit(modulus_lower_bound.get_mpz_t(), 0); //确保是一个奇数

    //初始化 m 个成员  或者从数据库直接初始化 members 成员，并修改其内部状态
    this->members.clear();
    this->available.clear();
    auto new_members = init_members(m);
    for(auto ele : new_members){
        members.insert(std::pair<mpz_class, PH_Member>(ele.get_modulus(), ele));
        //这里假设所有成员都可用
        available.insert(ele.get_modulus());
    }
    //初始化活跃组集合
    this->active_members.clear();
    
    //初始化 mod_product 和 lcm
    init_lcm_modproduct();

    //初始化 x 和 y
    init_xy();

    //初始化 active_mod_product active_lcm
    this->active_mod_product = 1;  //未有成员注册并加入活跃组
    this->active_lcm = 1;

    //初始化 m_key
    master_key_init();
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
    if(active_members.empty()) return;
    for(auto& ele : active_members){
        PH_Member& t = members[ele];
        this->m_key += t.get_enc_key() * t.get_x() * t.get_y();
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
    由 m_key 和 e_i 都是奇数， k * p_i 是偶数，又有 p_i 是素数，则 k 是偶数，于是 (k * p_i) mod (2 * p_i) = 0
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
}