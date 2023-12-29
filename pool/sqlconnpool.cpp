#include "sqlconnpool.h"
using namespace std;

SqlConnPool::SqlConnPool(){
    // useCount_ = 0;
    // freeCount_ = 0;
}

//单例模式
SqlConnPool* SqlConnPool::Instance(){
    static SqlConnPool connPool;
    return &connPool;
}

void SqlConnPool::Init(const char* host, int port,
                  const char* user, const char* pwd,
                  const char* dbName, int connSize){
    assert(connSize > 0);
    for(int i = 0; i < connSize; i++){
        MYSQL* sql = nullptr;
        sql = mysql_init(sql);//初始化连接句柄
        if(!sql){
            LOG_ERROR("MySql init error!");
            assert(sql);
        }
        sql = mysql_real_connect(sql, host, user, pwd, dbName, port, nullptr, 0);//连接服务器
        if(!sql){
            LOG_ERROR("MySql connect error!");
        }
        connQue_.push(sql);
    }
    MAX_CONN_ = connSize;
    sem_init(&semId_, 0, MAX_CONN_);
}

MYSQL* SqlConnPool::GetConn(){
    MYSQL* sql = nullptr;
    if(connQue_.empty()){
        LOG_WARN("SqlConnPool busy!");
        return nullptr;
    }
    sem_wait(&semId_);//请求一个连接
    {
        lock_guard<mutex> locker(mtx_);
        sql = connQue_.front();
        connQue_.pop();
    }
    return sql;
}

void SqlConnPool::FreeConn(MYSQL* conn){
    assert(conn);
    lock_guard<mutex> locker(mtx_);
    connQue_.push(conn);
    sem_post(&semId_);//释放资源
}

void SqlConnPool::ClosePool(){
    lock_guard<mutex> locker(mtx_);
    while(!connQue_.empty()){
        auto item = connQue_.front();
        connQue_.pop();
        mysql_close(item);//关闭mysql服务器的连接
    }
    mysql_library_end();//终止使用MySQL库
}

int SqlConnPool::GetFreeConnCount(){
    lock_guard<mutex> locker(mtx_);
    return connQue_.size();
}

SqlConnPool::~SqlConnPool(){
    ClosePool();
}