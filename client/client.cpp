#include "client.h"

Client::Client(int server_port, const char* server_ip):server_port_(server_port), server_ip_(server_ip){
    init();
}

Client::~Client(){
    if(client_sock_ >= 0){
        close(client_sock_);
    }
}

std::pair<std::string, std::string> Client::sendAndreceive(const std::string& title, const std::string& content){
    std::pair<std::string, std::string> message;
    int err = 0;
    writeBuf_.addMessage(title, content);
    writeBuf_.WriteFd(client_sock_, &err);
    readBuf_.ReadFd(client_sock_, &err);
    readBuf_.getMessage(message);
    return message;
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
    return true;
}



void Client::test(){


    writeBuf_.addMessage("allocation", "client");
    int err = 0;
    writeBuf_.WriteFd(client_sock_, &err);
    
    readBuf_.ReadFd(client_sock_, &err);
    std::pair<std::string, std::string> message1;
    readBuf_.getMessage(message1);
    std::cout << "Receive from server：<" << message1.first << ":" << message1.second << ">" << std::endl;
    readBuf_.getMessage(message1);
    std::cout << "Receive from server：<" << message1.first << ":" << message1.second << ">" << std::endl;

    message1 = sendAndreceive("mem_join", "client");
    std::cout << "Receive from server：<" << message1.first << ":" << message1.second << ">" << std::endl;

    message1 = sendAndreceive("mem_leave", "client");
    std::cout << "Receive from server：<" << message1.first << ":" << message1.second << ">" << std::endl;

    message1 = sendAndreceive("err", "client");
    std::cout << "Receive from server：<" << message1.first << ":" << message1.second << ">" << std::endl;
}