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

void Client::start(){
    if(isStop_) return;
    LOG_INFO("========== Client start ==========");
    std::pair<std::string, std::string> message;
    // LOG_DEBUG("trying login!");
    // writeBuf_.addMessage("login","yun-yun");
    // writeFd();
    // readFd(message);
    // std::cout << message.first << " : " << message.second << std::endl;

    LOG_DEBUG("trying register!");
    std::string user_info = user_name_ + "-" + passwd_;
    writeBuf_.addMessage("login", user_info);
    writeFd();
    readFd(message);
    std::cout << message.first << " : " << message.second << std::endl;
    if(message.second == "bad"){
        return;
    }

    if(!isregistered_){
        writeBuf_.addMessage("allocation", "client");//向密码系统注册
        writeFd();//向服务端发送
        readFd(message);
        LOG_DEBUG("<%s:%s>", message.first, message.second);
        std::cout << message.first << " : " << message.second << std::endl;
        if(message.second == "ok"){
            readFd(message);
            dec_key_ = mpz_class(message.second);
            LOG_DEBUG("dec_key = %s", dec_key_.get_str().c_str());
            std::cout << "dec_key = " << dec_key_.get_str() << std::endl;
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
    while(true){
        readFd(message);
        if(message.first == "rekeying"){
            gk_cipher_ = mpz_class(message.second);
            gk_ = decrypt(gk_cipher_);
            LOG_DEBUG("Receive from Server rekeying message: %s", gk_cipher_.get_str().c_str());
            std::cout << "Receive from Server rekeying message: " << gk_cipher_.get_str() << std::endl;
            LOG_DEBUG("new group key: %s", gk_.get_str().c_str());
            std::cout << "new group key: " << gk_.get_str() << std::endl;
        }else{
            LOG_DEBUG("Receive from Server unexpect message: %s---%s", message.first, message.second);
        }
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
    Log::Instance()->init(0, "./", "_client.log", 1024); 
    LOG_INFO("========== Client init ==========");
    return true;
}


int Client::readFd(std::pair<std::string, std::string>& message){
    int err;
    int ret = 0;
    while((ret = readBuf_.getMessage(message) == -2)){
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

mpz_class Client::decrypt(const mpz_class& ciphertext){
    mpz_class message;
    mpz_powm(message.get_mpz_t(), ciphertext.get_mpz_t(), dec_key_.get_mpz_t(), mod_.get_mpz_t());
    return message;
}