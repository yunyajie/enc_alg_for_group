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
}

void Conn::closeFd(){
    if(isClose_ == false){
        isClose_ = true;
        userCount--;
        //关闭当前连接对应的文件描述符
        close(fd_);
    }
    fd_ = -1;
}

PH_Member& Conn::getph_member(){
    return ph_member_;
}

ssize_t Conn::read(int* saveError){
    ssize_t len = -1;
    do{
        len = readBuff_.ReadFd(fd_, saveError);
        if(len <= 0){
            break;
        }
    }while(isET); //本项目默认是ET模式
    return len;
}

ssize_t Conn::write(int* saveError){
    //向文件描述符写
    return writeBuff_.WriteFd(fd_, saveError);
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