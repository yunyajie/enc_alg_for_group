#include "conn.h"

std::atomic<int> Conn::userCount;
bool Conn::isET;

Conn::Conn(){
}

Conn::~Conn(){
    if(fd_ > 0){
        closeFd();
    }
}

int Conn::getfd(){
    return fd_;
}

sockaddr_in Conn::getaddr(){
    return addr_;
}

void Conn::init(int fd, const sockaddr_in& addr){
    assert(fd > 0);
    userCount++;
    fd_ = fd;
    addr_ = addr;
    //清空缓冲区
    writeBuff_.RetrieveAll();
    readBuff_.RetrieveAll();
    //连接打开
    isClose_ = false;
    //还未成功登录
    islogin_ = false;
}

void Conn::closeFd(){
    if(isClose_ == false){
        isClose_ = true;
        islogin_ = false;
        userCount--;
        //关闭当前连接对应的文件描述符
        close(fd_);
    }
    fd_ = -1;
}

XH_Member& Conn::get_cipher_member(){
    return cipher_member_;
}

ssize_t Conn::read(int* saveError){//从文件描述符读取数据，直到把所有数据读完
    ssize_t len = 0;
    ssize_t ret = -1;
    do{
        ret = readBuff_.ReadFd(fd_, saveError);
        if(ret <= 0){//-1 表示发生错误， 0 表示数据读取完毕
            break;
        }else{
            len += ret;
        }
    }while(isET); //本项目默认是ET模式
    return ret < 0 ? ret : len; //ret < 0 表示发生了错误，如果没有发生错误，返回读到的字节数
}

ssize_t Conn::write(int* saveError){
    //向文件描述符写入缓冲区中所有数据
    int writesize = writeBuff_.ReadableBytes();
    int count = 0;
    int ret = 0;
    do{
        ret = writeBuff_.WriteFd(fd_, saveError);
        if(ret < 0){//发生错误
            return ret;
        }
        count += ret;
    }while(count < writesize);
    return count;
}

int Conn::getMessage(std::pair<std::string, std::string>& message){
    return readBuff_.getMessage(message);
}

void Conn::addMessage(const std::string& title, const std::string& content){
    writeBuff_.addMessage(title, content);
}

std::string Conn::GetReadStrNotRetrive(){
    return readBuff_.GetStrNotRetrieve();
}

void Conn::writeToBuff(std::string str){
    writeBuff_.Append(str);
}

//用户验证后会同时设置其内部的状态 userDbid_    设置 err -1 为数据库错误  -2 为用户密码错误 -3 为用户名已存在不可注册
bool Conn::userVerify(const std::string& name, const std::string& pwd, bool isLogin, int* err){
    if(name == "" || pwd == "") return false;
    LOG_INFO("Verify name: %s : pwd %s", name.c_str(), pwd.c_str());
    MYSQL* sql;
    SqlConnRAII(&sql, SqlConnPool::Instance());
    assert(sql);

    char order[256] = {0};
    MYSQL_FIELD* fields = nullptr;
    MYSQL_RES* res = nullptr;

    bool flag = false;
    if(!isLogin) flag = true;
    //查询用户及密码
    snprintf(order, 256, "SELECT name, password FROM users WHERE name='%s' LIMIT 1", name.c_str());
    LOG_DEBUG("%s", order);
    if(mysql_query(sql, order)){
        mysql_free_result(res);
        LOG_ERROR("%s", mysql_error(sql));
        *err = -1;
        return false;
    }
    res = mysql_store_result(sql);
    if(res == nullptr){
        mysql_free_result(res);
        LOG_ERROR("%s", mysql_error(sql));
        *err = -1;
        return false;
    }
    while(MYSQL_ROW row = mysql_fetch_row(res)){
        LOG_DEBUG("MYSQL ROW: %s %s", row[0], row[1]);
        std::string password(row[1]);
        if(isLogin){//登录行为
            if(pwd == password){
                LOG_DEBUG("Registered user login!")
                flag = true;
            }else{//密码错误
                flag = false;
                *err = -2;
                LOG_DEBUG("pwd error!");
            }
        }else{
            flag = false;
            *err = -3;
            LOG_DEBUG("user used!");
        }
    }
    mysql_free_result(res);
    //注册行为 且 用户名未被使用
    if(!isLogin && flag == true){
        LOG_DEBUG("register!");
        bzero(order, 256);
        snprintf(order, 256, "INSERT INTO users(name, password) VALUES('%s','%s')", name.c_str(), pwd.c_str());
        LOG_DEBUG("%s", order);
        if(mysql_query(sql, order)){
            flag = false;
            LOG_DEBUG("Insert error!");
        }else{
            flag = true;
            LOG_DEBUG("New user register success!");
        }
    }
    if(flag){
        userName_ = name;
        passwd_ = pwd;
        //查询用户Dbid
        snprintf(order, 256, "SELECT id FROM users WHERE name='%s' LIMIT 1", name.c_str());
        LOG_DEBUG("%s", order);
        if(mysql_query(sql, order)){
            mysql_free_result(res);
            return false;
        }
        res = mysql_store_result(sql);
        MYSQL_ROW row = mysql_fetch_row(res);
        LOG_DEBUG("MYSQL ROW: %s", row[0]);
        userDbid_ = atoi(row[0]);
        islogin_ = true;
        LOG_DEBUG("UserVerify success!");
    }else{
        userDbid_ = -1;
        islogin_ = false;
        LOG_DEBUG("UserVerify failure!");
    }
    return flag;
}

std::string Conn::getUserName(){
    return userName_;
}

int Conn::getUserDbid(){
    if(islogin_){
        return userDbid_;
    }
    return -1;
}