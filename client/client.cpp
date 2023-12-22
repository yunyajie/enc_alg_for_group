#include "client.h"

Client::Client(int server_port, const char* server_ip):server_port_(server_port), server_ip_(server_ip){
    init();
}

Client::~Client(){
    if(client_sock_ >= 0){
        close(client_sock_);
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
    return true;
}

void Client::test(){
    const char* message = "allocation";
    int cnt = 0;
    int ret = 0;

    //发送数据
    ret = send(client_sock_, message, strlen(message), 0);
    if(ret == -1){
        perror("send");
        close(client_sock_);
        return;
    }
    sleep(3);
    char buf[1024];
    ret = read(client_sock_, buf, 1024);
    std::cout << "Receive data from server: " << buf << std::endl;
    //发送数据
    ret = send(client_sock_, message, strlen(message), 0);
    sleep(10);
    ret = read(client_sock_, buf, 1024);
    std::cout << "Receive data from server: " << buf << std::endl;
}