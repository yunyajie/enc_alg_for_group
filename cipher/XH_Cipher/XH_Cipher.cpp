#include "XH_Cipher.h"
XH_Cipher::XH_Cipher(int m, int bit_length):_m(m), _bit_length(bit_length){
}

int XH_Cipher::allocation(Cipher_Member& new_register){
    if(_available.empty()){//密钥分配完毕，系统需要扩展
        //sys_extend();  //-----测试用，未连接数据库
        if(sys_extend_Db() == -1) return -1;
    }
    //从可用集合中分配密钥
    mpz_class t = *_available.begin();
    _available.erase(t);
    _members[t].registered();
    new_register = _members[t];
    LOG_INFO("Cipher available keys size : %d", _available.size());
    return 0;
}

mpz_class XH_Cipher::encrypt(const mpz_class& message){//每次加密都需要一个新的随机数 _r
    mpz_class cipher_text(0);
    mpz_class r;
    vector<string> strs;
    //首先初始化当前会话阶段的随机数 _r
    //使用当前时间的微秒级别信息作为种子
    auto seed = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    gmp_randstate_t state;
    //初始化 GMP 伪随机数生成器状态
    gmp_randinit_default(state);
    gmp_randseed_ui(state, static_cast<unsigned long>(seed));
    //生成一个 0 -- 2 ^ (2 * _bit_length - 1) 之间均匀分布的数     范围很大可保证其足够随机，长度并不影响结果
    mpz_urandomb(_r.get_mpz_t(), state, 2 * _bit_length);
    //清理状态
    gmp_randclear(state);
    for(auto& ele : _active_members){
        strs = {ele.get_str(), _r.get_str()};
        sha256encrypt(strs, r);
        //将组密钥嵌入到余数系统中
        r ^= message;
        _members[ele].set_r(r);
        cipher_text += _members[ele].get_r() * _members[ele].get_x() * _members[ele].get_y();
    }
    cipher_text %= _active_mod_product;
    return cipher_text;
}

int XH_Cipher::member_join(Cipher_Member& joiner){
    if(_members.find(joiner.get_modulus()) == _members.end()) return -1; //成员不在系统中
    //首先判断这个成员是否在活跃组中
    if(_active_members.find(joiner.get_modulus()) != _active_members.end()){
        //在活跃组中，主密钥不需要更新，直接返回
        return 0;
    }
    //更新成员状态为活跃
    joiner.active();
    _members[joiner.get_modulus()].active();
    //加入活跃组
    _active_members.insert(joiner.get_modulus());
    //更新系统参数
    _active_mod_product *= joiner.get_modulus();

    //更新主密钥信息参数和主密钥
    XH_Member&t = _members[joiner.get_modulus()];
    
    LOG_INFO("member join with mod %s", joiner.get_modulus().get_str().c_str());
    return 0;
}

int XH_Cipher::member_leave(Cipher_Member& leaver){     //注意这里的 leaver 的 x 和 y 参数并没有更新
    if(_members.find(leaver.get_modulus()) == _members.end()) return -1; //成员不在该系统中
    //首先判断这个成员是否在活跃组中
    if(_active_members.find(leaver.get_modulus()) == _active_members.end()){
        //成员不在活跃组中，直接返回
        return -1;
    }
    //成员在活跃组中，更新系统参数
    _active_mod_product /= leaver.get_modulus();
    //从活跃组中删除
    _active_members.erase(leaver.get_modulus());
    //更新成员状态为非活跃
    leaver.deactive();
    _members[leaver.get_modulus()].deactive();
    //更新主密钥信息参数和主密钥
    XH_Member&t = _members[leaver.get_modulus()];

    LOG_INFO("member leave with mod %s", leaver.get_modulus().get_str().c_str());
    return 0;
}

XH_Cipher::~XH_Cipher(){}

int XH_Cipher::active_size(){
    return _active_members.size();
}

mpz_class XH_Cipher::get_r(){
    return _r;
}

int XH_Cipher::sys_size(){
    return _members.size();
}

bool XH_Cipher::sys_init_fromDb(){
    LOG_INFO("XH_Cipher system init from DataBase!");
    //使用 bit_length 初始化 modulus_lower_bound 如果是从数据库载入，则取其中最大的模数为下界
    gmp_randstate_t state;
    gmp_randinit_default(state);    //初始化 GMP 伪随机数生成器状态
    mpz_urandomb(_modulus_lower_bound.get_mpz_t(), state, _bit_length);   //首先生成一个 bit_length 位的随机奇数
    mpz_setbit(_modulus_lower_bound.get_mpz_t(), _bit_length - 1); //确保是一个 bit_length 位长的数
    mpz_setbit(_modulus_lower_bound.get_mpz_t(), 0); //确保是一个奇数
    //从数据库初始化 members 成员，并修改其内部状态
    _members.clear();
    _available.clear();
    _active_members.clear();

    MYSQL* sql;
    SqlConnRAII(&sql, SqlConnPool::Instance());
    assert(sql);
    char order[256] = {0};
    MYSQL_FIELD* fields = nullptr;
    MYSQL_RES* res = nullptr;
    snprintf(order, 256, "SELECT modulus, used FROM xh_keys");
    LOG_DEBUG("%s", order);
    if(mysql_query(sql, order) == 1){
        LOG_ERROR("Select from database xh_cipher.xh_keys failed!");
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
        mpz_class mod(row[0]);
        int used = atoi(row[1]);
        XH_Member t(mod);
        if(used == 0){//密钥未被使用则为可用状态
            _available.insert(mod);
        }else{//已被使用的-----即已在密码系统中注册过的
            t.registered();
        }
        _members.insert(std::pair<mpz_class, XH_Member>(mod, t));
        _modulus_lower_bound = _modulus_lower_bound > mod ? _modulus_lower_bound : mod;
    }
    if(_members.size() == 0){//数据库是空的----生成新的密钥并添加到数据库中
        LOG_INFO("Database xh_cipher is empty, generating new keys for system and updating database!");
        auto new_members = init_members(_m);
        for(auto ele : new_members){
            _members.insert(std::pair<mpz_class, XH_Member>(ele.get_modulus(), ele));
            //新生成的密钥均为可用状态
            _available.insert(ele.get_modulus());
            snprintf(order, 256, "INSERT INTO xh_keys (modulus, used) VALUES ('%s', %d)", ele.get_modulus().get_str().c_str(), 0);
            LOG_DEBUG("%s", order);
            std::cout << order << std::endl;
            if(mysql_query(sql, order)){//插入新生成的密钥到数据库
                LOG_ERROR("Insert into database xh_cipher.xh_keys failed!");
                mysql_free_result(res);
                return false;
            }
        }
    }

    _m = _members.size();

    //初始化 _mod_product
    init_modproduct();

    //初始化 x 和 y
    init_xy();

    //初始化 _active_mod_product 此时还没有任何一个成员加入活跃组
    _active_mod_product = 1;       //未有成员注册并加入活跃组

    LOG_INFO("Cipher system size : %d", _members.size());
    LOG_INFO("Cipher available keys size : %d", _available.size());
    return true;
}

int XH_Cipher::sys_extend_Db(){
    LOG_INFO(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> XH_Cipher system extend <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<");
    std::vector<XH_Member> newmembers = init_members(_m);
    //将新的密钥加入到系统的成员集合中以及可分配集合中
    for(auto& ele : newmembers){
        _members.insert(std::make_pair(ele.get_modulus(), ele));
        _available.insert(ele.get_modulus());
    }
    //更新系统参数
    init_modproduct();
    init_xy();
    //更新 m
    _m *= 2;
    return 0;
}

void XH_Cipher::generate_prime(mpz_class& result_prime){
    mpz_nextprime(result_prime.get_mpz_t(), _modulus_lower_bound.get_mpz_t());
    _modulus_lower_bound = result_prime;
}

std::vector<XH_Member> XH_Cipher::init_members(int n){
    std::vector<XH_Member> new_members;
    mpz_class mod;
    for(int i = 0; i < n; i++){
        generate_prime(mod);
        new_members.emplace_back(mod);
    }
    return new_members;
}

void XH_Cipher::init_modproduct(){
    _mod_product = 1;
    for(auto& ele : _members){
        _mod_product *= ele.first;
    }
}



void XH_Cipher::init_xy(){
    mpz_class mod;
    mpz_class tem_x;
    mpz_class tem_y;
    for(auto& ele : _members){
        mod = ele.first;
        tem_x = _mod_product / mod;
        ele.second.set_x(tem_x);
        mpz_invert(tem_y.get_mpz_t(), tem_x.get_mpz_t(), mod.get_mpz_t());
        ele.second.set_y(tem_y);
    }
}