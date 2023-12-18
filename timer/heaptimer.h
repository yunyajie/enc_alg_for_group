#ifndef _HEAPTIMER_H_
#define _HEAPTIMER_H_

#include <vector>
#include <functional>
#include <chrono>
#include <unordered_map>
#include <assert.h>


typedef std::function<void()> TimeoutCallBack;
typedef std::chrono::high_resolution_clock Clock;
typedef std::chrono::milliseconds MS;
typedef Clock::time_point TimeStamp;

//定时器节点
struct TimerNode {
    int id;
    TimeStamp expires;          //时间到达的时间戳
    TimeoutCallBack cb;         //时间到达时的回调函数
    //重载 < 运算符用于比较函数
    bool operator<(const TimerNode& t) {
        return expires < t.expires;
    }
};
class HeapTimer {
    public:
        HeapTimer() { heap_.reserve(64); }

        ~HeapTimer() { clear(); }
        
        void adjust(int id, int newExpires);                        //调整指定id的结点

        void add(int id, int timeOut, const TimeoutCallBack& cb);   //添加新节点或修改已有节点

        void doWork(int id);                                        //删除指定id结点，并触发回调函数

        void clear();

        void tick();                                                //清除超时结点

        void pop();                                                 //删除队首结点

        int getNextTick();                                          //获取据下一个超时事件发生的时间

    private:
        void del_(size_t i);                        //删除指定位置的结点
        
        void siftup_(size_t i);                     //向上调整

        bool siftdown_(size_t index, size_t n);     //向下调整

        void SwapNode_(size_t i, size_t j);         //交换节点

        std::vector<TimerNode> heap_;

        std::unordered_map<int, size_t> ref_;       //保存节点 id 和其在堆中的下标的映射
};

#endif