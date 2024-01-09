#include "server.h"
using namespace std;

Server::Server(int port, 
    int sqlport, const char* sqluser, const char* sqlpwd, const char* dbName, int connPoolSize,
    int keylevel, int timeoutMs, bool openLog, int logLevel):port_(port), key_leavel_(keylevel), timeoutMs_(timeoutMs), isClose_(false), epoller_(new Epoller()), ph_cipher_(new PH_Cipher()){
    Conn::userCount = 0;
    Conn::isET = true;
    connEvent_ =  EPOLLONESHOT | EPOLLRDHUP | EPOLLET;
    //初始化监听套接字
    if(!initSock()){
        isClose_ = true;
    }

    //打开数据库连接池
    SqlConnPool::Instance()->Init("localhost", sqlport, sqluser, sqlpwd, dbName, connPoolSize);
    if(openLog){
        Log::Instance()->init(logLevel, "./log", "_server.log", 1024);
    }
    //初始化密码系统
    if(!ph_cipher_->sys_init_fromDb()){
        isClose_ = true;
        LOG_ERROR("========== Cipher System init error! ==========");
    }
    //打开日志
    if(openLog){
        if(isClose_){
            LOG_ERROR("========== Server init error! ==========");
        }else{
            LOG_INFO("========== Server init ==========");
            LOG_INFO("Port: %d, LogSys level: %d, GK Level: %d", port_, logLevel, key_leavel_);
        }
    }
}

Server::~Server(){
    close(listenfd_);
}

void Server::start(){
    if(!isClose_){ LOG_INFO("========== Server start =========="); }
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
                
                if(clients_[fd].getph_member().isActive()){//如果该用户是活跃组的成员但是掉线了，将该用户移出活跃组
                    ph_cipher_->member_leave(clients_[fd].getph_member());
                }
                //客户端断连，更新组密钥并向组内广播
                onWrite_newKey();
                LOG_DEBUG("new group key: %s", gk_.get_str().c_str());
                closeConn(&clients_[fd]);   //关闭连接
            }else if(events & EPOLLIN){
                assert(clients_.count(fd) > 0);
                //处理读操作-------新用户注册或旧用户加入
                dealRead(&clients_[fd]);
            }else if(events & EPOLLOUT){
                assert(clients_.count(fd) > 0);
                //处理写操作
                dealWrite(&clients_[fd]);
            }else{
                LOG_ERROR("Unexpected event!");
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
    LOG_INFO("Client[%d] in!", clients_[fd].getfd());
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
        LOG_INFO("Server receive data from client %d : < %s, %s >", client->getfd(), message.first.c_str(), message.second.c_str());
        cout << "Server receive data from client " << client->getfd() << " : " << message.first << " , "<< message.second << endl;
        if(message.first == "allocation"){//成员加入密码系统，如果是新注册的成员则分配新的密钥
            if(!onRead_allocation(client)){
                client->addMessage("allocation", "bad");
                LOG_ERROR("Client[%d] allocation failure!", client->getfd());
                epoller_->modFd(client->getfd(), EPOLLOUT | connEvent_);
            }
        }else if(message.first == "mem_join"){//成员加入活跃组
            if(0 == onRead_member_join(client)){
                onWrite_newKey();
            }else{
                LOG_ERROR("Client[%d] join failure!", client->getfd());
            }
        }else if(message.first == "mem_leave"){//成员离开活跃组
            if(0 == onRead_member_leave(client)){
                onWrite_newKey();
            }else{
                LOG_ERROR("Client[%d] leave failure!", client->getfd());
            }
        }else if(message.first == "login" || message.first == "register"){//成员登录和注册
            bool isLogin = (message.first == "login");
            onRead_userVerify(client, isLogin, message.second);
        }else{//非预期行为
            onRead_error(client);
        }
    }else if(ret == -2){//消息未完全接收
        epoller_->modFd(client->getfd(), EPOLLIN | connEvent_); //再次等待新的数据到来
    }else{//数据损坏，向客户端回送错误消息
        client->addMessage("err", "message damaged");
        epoller_->modFd(client->getfd(), EPOLLOUT | connEvent_);
        LOG_ERROR("Client message damaged!");
    }
}

void Server::onRead_userVerify(Conn* client, bool isLogin, const string& message){
    assert(client);
    int index = message.find('-');
    string name = message.substr(0, index);
    string pwd = message.substr(index + 1);
    if(client->userVerify(name, pwd, isLogin)){//向客户端返回注册和登录结果-----仅仅注册和登录不分配密钥
        client->addMessage("login", "ok");
    }else{
        client->addMessage("login", "bad");
    }
    epoller_->modFd(client->getfd(), connEvent_ | EPOLLOUT);
}

bool Server::onRead_allocation(Conn* client){
    assert(client);
    bool isNew = false;
    //ph_cipher_->allocation(client->getph_member());
    MYSQL* sql;
    SqlConnRAII(&sql, SqlConnPool::Instance());
    assert(sql);
    char order[256] = {0};
    MYSQL_FIELD* fields = nullptr;
    MYSQL_RES* res = nullptr;
    MYSQL_ROW row;
    snprintf(order, 256, "SELECT registered FROM users WHERE id=%d", client->getUserDbid());
    if(mysql_query(sql, order)){
        LOG_ERROR("%s", mysql_error(sql));
        return false;
    }
    res = mysql_store_result(sql);
    if(res == nullptr){
        LOG_ERROR("%s", mysql_error(sql));
        return false;
    }
    row = mysql_fetch_row(res);
    if(row){
        int registered = atoi(row[0]);
        if(registered == 0) isNew = true;
    }
    if(isNew){//新注册的成员
        if(ph_cipher_->allocation(client->getph_member()) == -1) return false;
        snprintf(order, 256, "SELECT key_id FROM ph_keys WHERE modulus=%s", client->getph_member().get_modulus().get_str().c_str());
        LOG_DEBUG("%s", order);
        if(mysql_query(sql, order)){
            LOG_ERROR("%s", mysql_error(sql));
            return false;
        }
        res = mysql_store_result(sql);
        if(res == nullptr){
            LOG_ERROR("%s", mysql_error(sql));
            mysql_free_result(res);
            return false;
        }
        row = mysql_fetch_row(res);
        int k_id = -1;
        if(row){
            k_id = atoi(row[0]);
        }else{
            return false;
        }
        snprintf(order, 256, "UPDATE users SET registered=%d, key_id=%d WHERE id=%d", 1, k_id, client->getUserDbid());//设置用户使用的密钥并将用户设置为已注册
        LOG_DEBUG("%s", order);
        if(mysql_query(sql, order)){
            LOG_ERROR("%s", mysql_error(sql));
            return false;
        }
        snprintf(order, 256, "UPDATE ph_keys SET used=%d WHERE key_id=%d", 1, k_id);//设置密钥为占用状态
        LOG_DEBUG("%s", order);
        if(mysql_query(sql, order)){
            LOG_ERROR("%s", mysql_error(sql));
            return false;
        }
    }else{//已分配密钥的成员
        snprintf(order, 256, "SELECT enc_key, modulus FROM all_info WHERE id=%d", client->getUserDbid());
        LOG_DEBUG("%s", order);
        if(mysql_query(sql, order)){
            LOG_ERROR("%s", mysql_error(sql));
            return false;
        }
        res = mysql_store_result(sql);
        if(res == nullptr){
            LOG_ERROR("%s", mysql_error(sql));
            mysql_free_result(res);
            return false;
        }
        row = mysql_fetch_row(res);
        if(row){
            mpz_class enc_key(row[0]);
            mpz_class mod(row[1]);
            client->getph_member() = PH_Member(enc_key, mod);
            client->getph_member().registered();
        }else{
            return false;
        }
    }
    //将解密密钥写入临时缓冲区
    client->addMessage("allocation", "ok");
    client->addMessage("dec_key", client->getph_member().get_dec_key().get_str());
    client->addMessage("mod", client->getph_member().get_modulus().get_str());
    epoller_->modFd(client->getfd(), EPOLLOUT | connEvent_);
    LOG_INFO("Client[%d] allocation!", client->getfd());
    return true;
}

int Server::onRead_member_join(Conn* client){//成员加入活跃组
    assert(client);
    int ret = 0;
    ret = ph_cipher_->member_join(client->getph_member());
    //向客户端会送加入成功或失败的消息
    if(ret == 0){//成功
        client->addMessage("mem_join", "ok");
        LOG_INFO("Client[%d] join in active group!", client->getfd());
    }else if(ret < 0){//加入失败
        client->addMessage("mem_join", "bad");
        LOG_ERROR("Client[%d] join error!", client->getfd());
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
        LOG_INFO("Client[%d] leave active group!", client->getfd());
    }else if(ret < 0){
        client->addMessage("mem_leave", "bad");
        LOG_ERROR("Client[%d] leave error!", client->getfd());
    }
    epoller_->modFd(client->getfd(), EPOLLOUT | connEvent_);    //向客户端回送消息
    return ret;
}

void Server::onRead_error(Conn* client){
    assert(client);
    LOG_ERROR("Client[%d] unexpected request event!", client->getfd());
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

void Server::onWrite_newKey(){
    generate_new_gk();
    mpz_class rekeymessage = ph_cipher_->encrypt(gk_);
    for(auto &ele : clients_){
        if(ele.second.getph_member().isActive()){
            ele.second.addMessage("rekeying", rekeymessage.get_str());
            epoller_->modFd(ele.second.getfd(), EPOLLOUT | connEvent_);
        }
    }
    LOG_INFO("Server send rekeying message: %s", rekeymessage.get_str().c_str());
}

void Server::generate_new_gk(){
    // 使用当前时间的微秒级别信息作为种子
    auto seed = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    gmp_randstate_t state;
    //初始化 GMP 伪随机数生成器状态
    gmp_randinit_default(state);
    gmp_randseed_ui(state, static_cast<unsigned long>(seed));
    mpz_urandomb(gk_.get_mpz_t(), state, key_leavel_);
    //清理状态
    gmp_randclear(state);
    LOG_DEBUG("Server select a new group key: %s", gk_.get_str().c_str());
}


void Server::closeConn(Conn* client){
    assert(client);
    LOG_INFO("Client[%d] quit!", client->getfd());
    epoller_->delFd(client->getfd());
    client->closeFd();
}