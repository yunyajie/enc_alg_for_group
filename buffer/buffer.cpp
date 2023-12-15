#include "buffer.h"

Buffer::Buffer(int initBuffSize) : buffer_(initBuffSize), readPos_(0), writePos_(0){}

//可读的字符数
size_t Buffer::ReadableBytes() const{
    return writePos_ - readPos_;
}

//可写的字符数
size_t Buffer::WritableBytes() const{
    return buffer_.size() - writePos_;
}

//返回已读的数据长度，用于缓冲区重整
size_t Buffer::PrependableBytes() const{
    return readPos_;
}

//返回下一个可读的位置
const char* Buffer::Peek() const{
    return BeginPtr_() + readPos_;
}

//将读指针前进 len 个单位
void Buffer::Retrieve(size_t len){
    assert(len <= ReadableBytes());
    readPos_ += len;
}

//将读指针前进到 end
void Buffer::RetrieveUntil(const char* end){
    assert(Peek() <= end);
    Retrieve(end - Peek());
}

//用0填满缓冲区
void Buffer::RetrieveAll() {
    bzero(&buffer_[0], buffer_.size());
    readPos_ = 0;
    writePos_ = 0;
}

//读取并以字符串返回，然后清空缓冲区
std::string Buffer::RetrieveAllToStr() {
    std::string str(Peek(), ReadableBytes());
    RetrieveAll();
    return str;
}

//下一个可写入的位置
const char* Buffer::BeginWriteConst() const {
    return BeginPtr_() + writePos_;
}

char* Buffer::BeginWrite() {
    return BeginPtr_() + writePos_;
}

//写入len长度后，更新下一个可写位置的下标
void Buffer::HasWritten(size_t len) {
    writePos_ += len;
}

//追加数据str
void Buffer::Append(const std::string& str) {
    Append(str.data(), str.length());
}

void Buffer::Append(const void* data, size_t len) {
    assert(data);
    Append(static_cast<const char*>(data), len);
}

void Buffer::Append(const char* str, size_t len) {
    assert(str);
    //确保有位置可写入
    EnsureWriteable(len);
    std::copy(str, str + len, BeginWrite());    //写入
    //写入后更新下一个可写入位置下标
    HasWritten(len);
}

//将Buffer中的数据追加
void Buffer::Append(const Buffer& buff) {
    Append(buff.Peek(), buff.ReadableBytes());
}

//确保有空间可写入，空间不够时拓展空间
void Buffer::EnsureWriteable(size_t len) {
    if(WritableBytes() < len) {
        MakeSpace_(len);
    }
    assert(WritableBytes() >= len);
}

//从文件描述符 fd 中读取数据
ssize_t Buffer::ReadFd(int fd, int* saveErrno) {
    char buff[65535];   //临时的数组，保证一次把所有的数据读取出来
    struct iovec iov[2];
    const size_t writable = WritableBytes();
    /* 分散读， 保证数据全部读完 */   //先往 Buffer 中读
    iov[0].iov_base = BeginPtr_() + writePos_;
    iov[0].iov_len = writable;
    iov[1].iov_base = buff; // Buffer 中装不下，再往临时的 buff 中读
    iov[1].iov_len = sizeof(buff);

    const ssize_t len = readv(fd, iov, 2);  //读取数据到多个缓冲区
    if(len < 0) {
        *saveErrno = errno;     //返回错误号
    }
    else if(static_cast<size_t>(len) <= writable) {
        // Buffer 中装得下
        writePos_ += len;
    }
    else {
        // Buffer 中装不下，已经装满了，先更新可写入的下一个位置的下标，然后将临时数组中的数据追加到缓冲区中
        writePos_ = buffer_.size();
        Append(buff, len - writable);
    }
    return len;
}

//向文件描述符 fd 中写入数据
ssize_t Buffer::WriteFd(int fd, int* saveErrno) {
    size_t readSize = ReadableBytes();
    ssize_t len = write(fd, Peek(), readSize);
    if(len < 0) {
        *saveErrno = errno;
        return len;
    } 
    readPos_ += len;
    return len;
}

//返回buffer的首地址
char* Buffer::BeginPtr_() {
    return &*buffer_.begin();
}

const char* Buffer::BeginPtr_() const {
    return &*buffer_.begin();
}

//扩容缓冲区
void Buffer::MakeSpace_(size_t len) {
    //可写的区域  +  已读的区域
    if(WritableBytes() + PrependableBytes() < len) {//小于要写入的数据量，扩容
        //整个数组都不够装，扩容数组
        buffer_.resize(writePos_ + len + 1);
    } 
    else {//大于要写入的数据量，整个数组装得下，将可读的数据整体挪到数组的最前面覆盖掉已读取的数据
        size_t readable = ReadableBytes();
        std::copy(BeginPtr_() + readPos_, BeginPtr_() + writePos_, BeginPtr_());
        readPos_ = 0;
        writePos_ = readPos_ + readable;
        assert(readable == ReadableBytes());
    }
}