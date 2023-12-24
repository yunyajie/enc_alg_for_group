#include "server.h"
using namespace std;

Server::Server(int port):port_(port), isClose_(false), epoller_(new Epoller()), ph_cipher_(new PH_Cipher()){
    Conn::userCount = 0;
    Conn::isET = true;
    connEvent_ =  EPOLLONESHOT | EPOLLRDHUP | EPOLLET;
    //初始化监听套接字
    if(!initSock()){
        isClose_ = true;
    }
}

Server::~Server(){
    close(listenfd_);
}

void Server::start(){
    if(!isClose_) cout << "==============Server start==============" << endl;
    while(!isClose_){
        //默认无事件将阻塞
        int eventCnt = epoller_->wait();
        for(int i = 0; i < eventCnt; i++){
            int fd = epoller_->getEventFd(i);
            uint32_t events = epoller_->getEvents(i);
            //监听文件描述符
            if(fd == listenfd_){
                dealListen();  //处理监听操作----接受新的连接
            }else if(events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)){
                assert(clients_.count(fd) > 0);
                //客户端断连，更新主密钥并向组内广播

                //向客户端回送错误发生
                closeConn(&clients_[fd]);   //关闭连接
            }else if(events & EPOLLIN){
                assert(clients_.count(fd) > 0);
                //处理读操作-------新用户注册或旧用户加入
                dealRead(&clients_[fd]);

                //重新添加读事件到内核事件表
                //epoller_->modFd(fd, EPOLLIN | EPOLLET | EPOLLRDHUP | EPOLLONESHOT);
            }else if(events & EPOLLOUT){
                assert(clients_.count(fd) > 0);
                cout << "输出事件触发" << endl;
                //处理写操作-------向组内发布新的组密钥或向新成员发送它的私钥
                dealWrite(&clients_[fd]);

            }else{
                cout << "Unexpected event!" <<endl;
            }
        }
    }
}

//初始化监听文件描述符
bool Server::initSock(){
    //创建监听套接字
    listenfd_ = socket(AF_INET, SOCK_STREAM, 0);
    if(listenfd_ < 0){
        perror("socket");
        return false;
    }

    int ret = 0;
    //设置地址重用
    int flag = 1;
    ret = setsockopt(listenfd_, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));
    if(ret < 0){
        perror("setsockopt");
        close(listenfd_);
        return false;
    }

    //绑定地址
    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY); //服务器可绑定任意地址
    addr.sin_port = htons(port_);
    ret = bind(listenfd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
    if(ret < 0){
        perror("bind");
        close(listenfd_);
        return false;
    }

    //监听连接
    ret = listen(listenfd_, 5);
    if(ret < 0){
        perror("listen");
        close(listenfd_);
        return false;
    }

    //添加 listenfd_ 上的输入事件
    if(!epoller_->addFd(listenfd_, EPOLLRDHUP | EPOLLIN | EPOLLET)){
        perror("epoll_ctl_add");
        close(listenfd_);
        return false;
    }
    setFdNonblock(listenfd_);
    return true;
}

int Server::setFdNonblock(int fd){
    assert(fd > 0);
    return fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK);
}

void Server::addClient(int fd, sockaddr_in addr){
    assert(fd > 0);
    clients_[fd].init(fd, addr);
    //监听客户端的读事件
    epoller_->addFd(fd, EPOLLIN | connEvent_);
    setFdNonblock(fd);  //设置为非阻塞
}

void Server::dealListen(){
    //保存客户端的信息
    sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);
    //listenfd 的模式默认是 ET,需要一次性把所有数据全部读取出来
    while(true){
        int fd = accept(listenfd_, reinterpret_cast<sockaddr*>(&addr), &addr_len);
        if(fd <= 0) return;
        addClient(fd, addr);    //添加客户端
    }
}

void Server::dealRead(Conn* client){
    assert(client);
    int ret = -1;
    int readError = 0;
    ret = client->read(&readError);
    if(ret <= 0 && readError != EAGAIN){
        closeConn(client);
        return;
    }
    //处理读事件
    int err = 0;
    client->read(&err);
    //获取读到的消息决定客户端要求
    pair<string, string> message;
    ret = client->getMessage(message);
    if(ret == 0){//获得一条完整的消息
        cout << "Server receive data from client " << client->getfd() << " : <" << message.first << ":" << message.second << ">" << endl;
        if(message.first == "allocation"){
            //成员注册
            onRead_allocation(client);
        }else if(message.first == "mem_join"){
            //成员加入活跃组
            onRead_member_join(client);
        }else if(message.first == "mem_leave"){
            //成员离开活跃组
            onRead_member_leave(client);
        }else{
            //非预期行为
            onRead_error(client);
        }
    }else if(ret == -2){//消息未完全接收
        epoller_->modFd(client->getfd(), EPOLLIN | connEvent_); //再次等待新的数据到来
    }else{
        //数据损坏，关闭客户端连接
        cout << "数据损坏" << endl;
        //向客户端回送错误消息
        client->addMessage("err", "message damaged");
        epoller_->modFd(client->getfd(), EPOLLOUT | connEvent_);
        //closeConn(client);
    }
}

void Server::onRead_allocation(Conn* client){
    assert(client);
    ph_cipher_->allocation(client->getph_member());
    //将解密密钥写入临时缓冲区
    client->addMessage("dec_key", client->getph_member().get_dec_key().get_str());
    client->addMessage("modulus", client->getph_member().get_modulus().get_str());
    epoller_->modFd(client->getfd(), EPOLLOUT | connEvent_);
}

int Server::onRead_member_join(Conn* client){//成员加入活跃组
    assert(client);
    int ret = 0;
    ret = ph_cipher_->member_join(client->getph_member());
    //向客户端会送加入成功或失败的消息
    if(ret == 0){//成功
        client->addMessage("mem_join", "ok");
    }else if(ret < 0){//加入失败
        client->addMessage("mem_join", "bad");
    }
    epoller_->modFd(client->getfd(), EPOLLOUT | connEvent_);    //向客户端回送消息
    return ret;
}

int Server::onRead_member_leave(Conn* client){//成员离开活跃组
    assert(client);
    int ret = ph_cipher_->member_leave(client->getph_member());
    //向客户端会送加入成功或失败的消息
    if(ret == 0){
        client->addMessage("mem_leave", "ok");
    }else if(ret < 0){
        client->addMessage("mem_leave", "bad");
    }
    epoller_->modFd(client->getfd(), EPOLLOUT | connEvent_);    //向客户端回送消息
    return ret;
}

void Server::onRead_error(Conn* client){
    assert(client);
    cout << "Unexpected EPOLLIN event!" << endl;
    client->addMessage("err", "Unexpected behaviour!");
    epoller_->modFd(client->getfd(), EPOLLOUT | connEvent_);    //向客户端回送消息
}

void Server::dealWrite(Conn* client){
    assert(client);
    //处理写事件
    int err = 0;
    client->write(&err);
    if(err & EAGAIN){//不可写，没写完，尝试再次写
        epoller_->modFd(client->getfd(), EPOLLOUT | connEvent_);
    }else{//写完了，等待读事件
        epoller_->modFd(client->getfd(), EPOLLIN | connEvent_);
    }
}


void Server::closeConn(Conn* client){
    assert(client);
    epoller_->delFd(client->getfd());
    client->closeFd();
}