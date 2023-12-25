#ifndef _BUFFER_H_
#define _BUFFER_H_
#include <cstring>
#include <iostream>
#include <vector>
#include <atomic>
#include <assert.h>
#include <sys/uio.h>
#include <unistd.h>

class Buffer{
    public:
        Buffer(int initBuffSize = 1024);
        ~Buffer() = default;                    //删除缺省的析构函数

        size_t WritableBytes() const;           //可写的字符数
        size_t ReadableBytes() const;           //可读的字符数
        size_t PrependableBytes() const;        //返回已读字符串的长度，用于缓冲区中字符的重整（用未读数据覆盖已读数据，使得缓冲区后方空出位置读取新来的数据）

        const char* Peek() const;               //下一个可读的指针位置
        void EnsureWriteable(size_t len);       //确保有空间可写入，空间不够时拓展空间
        void HasWritten(size_t len);            //写入len长度后，更新下一个可写位置的下标 writePos_

        void Retrieve(size_t len);              //将读指针前进 len 个单位
        void RetrieveUntil(const char* end);    //将读指针前进到 end
        std::string RetrieveUntilToStr(const char* end); //将读指针前进到 end，并将这之间的字符串返回

        void RetrieveAll() ;
        std::string RetrieveAllToStr();
        std::string GetStrNotRetrieve();             //获取缓冲区中的信息并返回但不改读写指针

        const char* BeginWriteConst() const;        //写一个可写入的位置
        char* BeginWrite();

        void Append(const std::string& str);        //向缓冲区中追加数据
        void Append(const char* str, size_t len);
        void Append(const void* data, size_t len);
        void Append(const Buffer& buff);
        void Append(const char& c);

        ssize_t ReadFd(int fd, int* Errno);         //从文件描述符 fd 中读取数据
        ssize_t WriteFd(int fd, int* Errno);        //向文件描述符 fd 中写入数据

        int getMessage(std::pair<std::string, std::string>& message);       //从缓冲获取一条完整消息对， <消息头，消息体>
        void addMessage(const std::string& title, const std::string& content);      //向缓冲区写入一条完整消息

        static const char border;                        //消息边界
        static const char delimiter;                     //消息头和内容的分隔符

    private:
        char* BeginPtr_();                          //缓冲区起始位置指针
        const char* BeginPtr_() const;
        char* Peek_();                         //下一个可读的指针位置
        void MakeSpace_(size_t len);                //确保缓冲区可以装下 len 长的字符串

        std::vector<char> buffer_;                  //缓冲区
        std::atomic<std::size_t> readPos_;          //可读的起始位置
        std::atomic<std::size_t> writePos_;         //可写的起始位置

};

#endif