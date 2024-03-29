#include "client.h"

Client::Client(int server_port, const char* server_ip, const char* userName, const char* passwd)
:server_port_(server_port), server_ip_(server_ip), isactive_(false), isregistered_(false), user_name_(userName), passwd_(passwd){
    if(init()){
        isStop_ = false;
    }else{
        isStop_ = true;
    }
}

Client::~Client(){
    if(client_sock_ >= 0){
        close(client_sock_);
    }
}

void Client::start(){//先尝试登录---登录失败时再注册
    if(isStop_) return;
    LOG_INFO("========== Client start ==========");
    std::pair<std::string, std::string> message;
    std::string user_info = user_name_ + "-" + passwd_;
    LOG_DEBUG("trying login!");
    writeBuf_.addMessage("login", user_info);
    writeFd();
    readFd(message);
    std::cout << message.first << " : " << message.second << std::endl;
    if(message.second == "bad"){
        LOG_DEBUG("trying register!");
        writeBuf_.addMessage("register", user_info);
        writeFd();
        readFd(message);
        std::cout << message.first << " : " << message.second << std::endl;
        if(message.second == "bad"){
            LOG_DEBUG("user %s already in system!", user_name_.c_str());
            return;
        }
        isregistered_ = false;
    }else if(message.second == "pwd"){
        LOG_DEBUG("passwd error!");
        return;
    }
    isregistered_ = true;
    if(isregistered_){
        writeBuf_.addMessage("allocation", "client");//向密码系统注册
        writeFd();//向服务端发送
        readFd(message);
        LOG_DEBUG("<%s:%s>", message.first.c_str(), message.second.c_str());
        std::cout << message.first << " : " << message.second << std::endl;
        if(message.second == "ok"){
            readFd(message);
            mod_ = mpz_class(message.second);
            LOG_DEBUG("mod = %s", mod_.get_str().c_str());
            std::cout << "mod = " << mod_.get_str() << std::endl;
            isregistered_ = true;
        }else{
            isregistered_ = false;
            return;
        }
        
    }
    if(!isactive_){//加入活跃组
        writeBuf_.addMessage("mem_join", "client");
        writeFd();
        readFd(message);
        std::cout << message.first << " : " << message.second << std::endl;
        if(message.second == "ok"){
            isactive_ = true;
            LOG_INFO("Client join the active group!");
            std::cout << "Client join the active group!" << std::endl;
        }else{
            LOG_ERROR("Client cannot join the active group!");
            std::cout << "Client cannot join the active group!" << std::endl;
            return;
        }
    }
    int err;
    while(!isStop_){
        readFd(message);
        if(message.first == "rekeying"){
            gk_cipher_ = mpz_class(message.second);
            LOG_DEBUG("Receive from Server rekeying message: %s", gk_cipher_.get_str().c_str());
            std::cout << "Receive from Server rekeying message: " << gk_cipher_.get_str() << std::endl;
        }else{
            LOG_DEBUG("Receive from Server unexpect message: %s---%s", message.first, message.second);
        }
        readFd(message);
        if(message.first == "randomNum"){
            r_ = mpz_class(message.second);
            LOG_DEBUG("Receive from Server RandomNum: %s", r_.get_str().c_str());
            std::cout << "Receive from Server RandomNum: " << r_.get_str() << std::endl;
        }else{
            LOG_DEBUG("Receive from Server unexpect message: %s---%s", message.first, message.second);
        }
        gk_ = decrypt();
        LOG_DEBUG("new group key: %s", gk_.get_str().c_str());
        std::cout << "new group key: " << gk_.get_str() << std::endl;
    }
}


bool Client::init(){
    //初始化客户端套接字
    client_sock_ = socket(AF_INET, SOCK_STREAM, 0);
    assert(client_sock_ >= 0);
    //设置服务器地址
    memset(&server_addr_, 0, sizeof(server_addr_));
    server_addr_.sin_family = AF_INET;
    server_addr_.sin_addr.s_addr = inet_addr(server_ip_);
    server_addr_.sin_port = htons(server_port_);
    //连接到服务器
    int ret = connect(client_sock_, reinterpret_cast<sockaddr*>(&server_addr_), sizeof(server_addr_));
    if(ret == -1){
        perror("connect");
        close(client_sock_);
        return false;
    }
    Log::Instance()->init(0, "./log", "_client.log", 1024); 
    LOG_INFO("========== Client init ==========");
    return true;
}


int Client::readFd(std::pair<std::string, std::string>& message){
    int err;
    int ret = 0;
    while((ret = readBuf_.getMessage(message)) == -2){
        int rett = readBuf_.ReadFd(client_sock_, &err);
        if(rett < 0){
            if(err & EINTR) continue;//接着尝试读
            else{//服务端关闭连接或者出错
                LOG_ERROR("Server is down!");
                std::cout << "Server is down!" << std::endl;
                return -1;
            }
        }
    }
    return ret;
}

int Client::writeFd(){
    int ret = 0;
    int err;
    while((ret = writeBuf_.WriteFd(client_sock_, &err)) <= 0){
        if(err & EINTR) continue; //接着写
        else{
            LOG_ERROR("Server is down!");
            return -1;
        }
    }
    return 0;
}

mpz_class Client::decrypt(){
    mpz_class md;
    //先获取余数
    gk_ = gk_cipher_ % mod_;
    //获取哈希值
    std::vector<string> strs = {mod_.get_str(), r_.get_str()};
    sha256encrypt(strs, md);
    gk_ ^= md;
    return gk_;
}

mpz_class Client::get_gk(){
    return gk_;
}