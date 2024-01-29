#include "PH_Cipher.h"
//PH_Cipher 的实现

PH_Cipher::PH_Cipher(int m, int bit_length):m(m), bit_length(bit_length){
    //系统初始化-----测试用，未连接数据库
    //sys_init();
}

//新成员注册，系统分配可用密钥
int PH_Cipher::allocation(PH_Member& new_register){
    if(available.empty()){//密钥分配完毕，系统需要扩展
        //sys_extend();  //-----测试用，未连接数据库
        if(sys_extend_Db() == -1) return -1;
    }
    //从可用集合中分配密钥
    mpz_class t = *available.begin();
    available.erase(t);
    this->members[t].registered();
    new_register = this->members[t];
    LOG_INFO("Cipher available keys size : %d", this->available.size());
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
    //更新成员状态为活跃
    joiner.active();
    members[joiner.get_modulus()].active();
    //加入活跃组
    active_members.insert(joiner.get_modulus());
    //更新系统参数
    this->active_exp_mod_product *= (joiner.get_modulus() - 1) / 2;
    this->active_mod_product *= joiner.get_modulus();

    //更新主密钥信息参数和主密钥
    PH_Member&t = members[joiner.get_modulus()];
    this->m_key_info += t.get_enc_key() * t.get_x() * t.get_y();
    this->m_key_info %= this->exp_mod_product;      //降低累加和的存储空间
    this->m_key = this->m_key_info % this->active_exp_mod_product;
    
    LOG_INFO("member join with mod %s", joiner.get_modulus().get_str().c_str());
    LOG_INFO("master key refresh %s", this->m_key.get_str().c_str());
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
    this->active_exp_mod_product /= (leaver.get_modulus() - 1) / 2;
    this->active_mod_product /= leaver.get_modulus();
    //从活跃组中删除
    active_members.erase(leaver.get_modulus());
    //更新成员状态为非活跃
    leaver.deactive();
    members[leaver.get_modulus()].deactive();
    //更新主密钥信息参数和主密钥
    PH_Member&t = members[leaver.get_modulus()];
    this->m_key_info -= t.get_enc_key() * t.get_x() * t.get_y();
    this->m_key_info = (this->m_key_info % this->exp_mod_product + this->exp_mod_product) % this->exp_mod_product;      //降低累加和的存储空间
    this->m_key = this->m_key_info % this->active_exp_mod_product;

    LOG_INFO("member leave with mod %s", leaver.get_modulus().get_str().c_str());
    LOG_INFO("master key refresh %s", this->m_key.get_str().c_str());
    return 0;
}

int PH_Cipher::active_size(){
    return active_members.size();
}

int PH_Cipher::sys_size(){
    return members.size();
}

PH_Cipher::~PH_Cipher(){}

//系统扩展为原来的两倍并更新数据库
int PH_Cipher::sys_extend_Db(){
    LOG_INFO(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> PH_Cipher system extend with Database Update <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<");
    std::vector<PH_Member> newmembers = init_members(this->m);
    MYSQL* sql;
    SqlConnRAII(&sql, SqlConnPool::Instance());
    assert(sql);
    char order[256] = {0};
    MYSQL_FIELD* fields = nullptr;
    MYSQL_RES* res = nullptr;
    //将新的密钥加入到系统的成员集合中以及可分配集合中
    for(auto& ele : newmembers){
        members.insert(std::make_pair(ele.get_modulus(), ele));
        available.insert(ele.get_modulus());
        snprintf(order, 256, "INSERT INTO ph_keys (modulus, enc_key, dec_key, used) VALUES ('%s', '%s', '%s', %d)",
             ele.get_modulus().get_str().c_str(), ele.get_enc_key().get_str().c_str(), ele.get_dec_key().get_str().c_str(), 0);
        LOG_DEBUG("%s", order);
        std::cout << order << std::endl;
        if(mysql_query(sql, order)){//插入
            LOG_ERROR("Insert into database ph_cipher.ph_keys failed!");
            mysql_free_result(res);
            return -1;
        }
    }
    //更新系统参数
    init_modproduct();
    init_xy();
    //更新 m
    this->m *= 2;
    //初始化 m_key 和 m_key_info
    master_key_init();
    return 0;
}

//系统扩展为原来的两倍
int PH_Cipher::sys_extend(){
    LOG_INFO(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> PH_Cipher system extend <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<");
    std::vector<PH_Member> newmembers = init_members(this->m);
    //将新的密钥加入到系统的成员集合中以及可分配集合中
    for(auto& ele : newmembers){
        members.insert(std::make_pair(ele.get_modulus(), ele));
        available.insert(ele.get_modulus());
    }
    //更新系统参数
    init_modproduct();
    init_xy();
    //更新 m
    this->m *= 2;
    //初始化 m_key 和 m_key_info
    master_key_init();
    return 0;
}

//初始化 mod_product 和 exp_mod_product
void PH_Cipher::init_modproduct(){
    this->mod_product = 1;
    this->exp_mod_product = 2;
    for(auto& ele : this->members){
        mpz_class mod = ele.first;
        this->mod_product *= mod;
        this->exp_mod_product *= static_cast<mpz_class>((mod - 1) / 2);
    }
}

//初始化 x 和 y
void PH_Cipher::init_xy(){
    mpz_class tem_x, tem_y;
    for(auto& ele : this->members){
        tem_x = exp_mod_product / static_cast<mpz_class>((ele.first - 1) / 2);
        ele.second.set_x(tem_x);
        mpz_invert(tem_y.get_mpz_t(), tem_x.get_mpz_t(), static_cast<mpz_class>((ele.first - 1) / 2).get_mpz_t());
        ele.second.set_y(tem_y);
    }
}

//系统初始化
void PH_Cipher::sys_init(){
    LOG_INFO("PH_Cipher system init!");
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
    
    //初始化 mod_product 和 exp_mod_product
    init_modproduct();

    //初始化 x 和 y
    init_xy();

    //初始化 active_mod_product active_exp_mod_product  此时还没有任何一个成员加入活跃组
    this->active_mod_product = 1;       //未有成员注册并加入活跃组
    this->active_exp_mod_product = 2;   //最终分摊到每个成员是在指数上模除 2 * m_i，故默认模数为 2 的影子成员在活跃组中

    //初始化 m_key 和 m_key_info
    master_key_init();
}

bool PH_Cipher::sys_init_fromDb(){
    LOG_INFO("PH_Cipher system init from DataBase!");
    //使用 bit_length 初始化 modulus_lower_bound 如果是从数据库载入，则取其中最大的模数为下界
    gmp_randstate_t state;
    gmp_randinit_default(state);    //初始化 GMP 伪随机数生成器状态
    mpz_urandomb(modulus_lower_bound.get_mpz_t(), state, bit_length);   //首先生成一个 bit_length 位的随机奇数
    mpz_setbit(modulus_lower_bound.get_mpz_t(), bit_length - 1); //确保是一个 bit_length 位长的数
    mpz_setbit(modulus_lower_bound.get_mpz_t(), 0); //确保是一个奇数
    //从数据库初始化 members 成员，并修改其内部状态
    this->members.clear();
    this->available.clear();
    this->active_members.clear();

    MYSQL* sql;
    SqlConnRAII(&sql, SqlConnPool::Instance());
    assert(sql);
    char order[256] = {0};
    MYSQL_FIELD* fields = nullptr;
    MYSQL_RES* res = nullptr;
    snprintf(order, 256, "SELECT enc_key, modulus, used FROM ph_keys");
    LOG_DEBUG("%s", order);
    if(mysql_query(sql, order) == 1){
        LOG_ERROR("Select from database ph_cipher.ph_keys failed!");
        mysql_free_result(res);
        return false;
    }
    res = mysql_store_result(sql);
    if(res == NULL){
        LOG_ERROR("mysql_store_result failed!");
        mysql_free_result(res);
        return false;
    }
    while(MYSQL_ROW row = mysql_fetch_row(res)){
        mpz_class enc_key(row[0]);
        mpz_class mod(row[1]);
        int used = atoi(row[2]);
        PH_Member t(enc_key, mod);
        if(used == 0){//密钥未被使用则为可用状态
            available.insert(mod);
        }else{//已被使用的-----即已在密码系统中注册过的
            t.registered();
        }
        members.insert(std::pair<mpz_class, PH_Member>(mod, t));
        modulus_lower_bound = modulus_lower_bound > mod ? modulus_lower_bound : mod;
    }
    if(members.size() == 0){//数据库是空的----生成新的密钥并添加到数据库中
        LOG_INFO("Database ph_cipher is empty, generating new keys for system and updating database!");
        auto new_members = init_members(m);
        for(auto ele : new_members){
            members.insert(std::pair<mpz_class, PH_Member>(ele.get_modulus(), ele));
            //新生成的密钥均为可用状态
            available.insert(ele.get_modulus());
            snprintf(order, 256, "INSERT INTO ph_keys (modulus, enc_key, dec_key, used) VALUES ('%s', '%s', '%s', %d)",
             ele.get_modulus().get_str().c_str(), ele.get_enc_key().get_str().c_str(), ele.get_dec_key().get_str().c_str(), 0);
            LOG_DEBUG("%s", order);
            std::cout << order << std::endl;
            if(mysql_query(sql, order)){//插入新生成的密钥到数据库
                LOG_ERROR("Insert into database ph_cipher.ph_keys failed!");
                mysql_free_result(res);
                return false;
            }
        }
    }

    this->m = members.size();

    //初始化 mod_product 和 exp_mod_product
    init_modproduct();

    //初始化 x 和 y
    init_xy();

    //初始化 active_mod_product active_exp_mod_product  此时还没有任何一个成员加入活跃组
    this->active_mod_product = 1;       //未有成员注册并加入活跃组
    this->active_exp_mod_product = 2;   //最终分摊到每个成员是在指数上模除 2 * m_i，故默认模数为 2 的影子成员在活跃组中

    //初始化 m_key 和 m_key_info
    master_key_init();

    LOG_INFO("Cipher system size : %d", this->members.size());
    LOG_INFO("Cipher available keys size : %d", this->available.size());
    return true;
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

//获取一个随机的加密密钥 其值 < upperLimit，满足条件是 e = 2 * r + 1，即一个奇数，且小于其对应的模数
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

//系统初始化后初始化 m_key 和 m_key_info
void PH_Cipher::master_key_init(){
    //根据 active_members 中的活跃成员初始化 m_key 和 m_key_info
    this->m_key_info = 0;
    //模数为 2 假设为一个影子成员
    mpz_class shadow_user_x = this->exp_mod_product / 2;
    mpz_class shadow_user_y;
    mpz_invert(shadow_user_y.get_mpz_t(), shadow_user_x.get_mpz_t(), static_cast<mpz_class>(2).get_mpz_t());
    this->m_key_info = 1 * shadow_user_x * shadow_user_y;
    for(auto& ele : active_members){
        PH_Member& t = members[ele];
        this->m_key_info += t.get_enc_key() * t.get_x() * t.get_y();
    }
    this->m_key_info %= this->exp_mod_product;      //降低累加和的存储空间
    this->m_key = this->m_key_info % this->active_exp_mod_product;
    LOG_INFO("master key init %s", this->m_key.get_str().c_str());
}