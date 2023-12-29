#ifndef _SQLCONNPOOL_H_
#define _SQLCONNPOOL_H_

#include <mysql/mysql.h>
#include <queue>
#include <semaphore.h>
#include <assert.h>
#include "../log/log.h"

class SqlConnPool{
    public:
        static SqlConnPool* Instance();
        MYSQL* GetConn();
        void FreeConn(MYSQL* conn);
        int GetFreeConnCount();

        void Init(const char* host, int port,
                  const char* user, const char* pwd,
                  const char* dbName, int connSize = 10);
        void ClosePool();
    private:
        SqlConnPool();
        ~SqlConnPool();         //这样只能通过成员函数来销毁对象

        int MAX_CONN_;                  //最大连接数
        // int useCount_;                  //已使用的连接数
        // int freeCount_;                 //空闲连接数

        std::queue<MYSQL*> connQue_;    //连接指针队列
        std::mutex mtx_;                //互斥量保护队列
        sem_t semId_;                   //信号量记录资源使用情况
};

#endif