//#include "PH_Cipher.h"
#include <vector>
#include <cstring>
#include <iostream>
#include <cstdlib>
#include <unistd.h>
#include <string>
#include <arpa/inet.h>

int main() {
    const char* server_ip = "172.18.0.1";
    const int server_port = 8000;

    //创建客户端套接字
    int clientSock = socket(AF_INET, SOCK_STREAM, 0);
    //设置服务器地址
    sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(server_ip);
    server_addr.sin_port = htons(server_port);

    //连接到服务器
    int ret = connect(clientSock, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr));
    if(ret == -1){
        perror("connect");
        close(clientSock);
        return 1;
    }
    const char* message = "Hello, Server!\n";
    int cnt = 0;
    std::string str;
    while(true){
        //发送数据
        ret = send(clientSock, message, strlen(message), 0);
        if(ret == -1){
            perror("send");
            close(clientSock);
            return 1;
        }
        sleep(3);
    }

    // 关闭套接字
    close(clientSock);

    return 0;
}
