#ifndef _HEAPTIMER_H_
#define _HEAPTIMER_H_

#include <vector>
#include <functional>
#include <chrono>
#include <unordered_map>
#include <assert.h>
#include "../log/log.h"


typedef std::function<void()> TimeoutCallBack;
typedef std::chrono::high_resolution_clock Clock;
typedef std::chrono::milliseconds MS;
typedef Clock::time_point TimeStamp;

//定时器节点
struct TimerNode {
    int id;
    TimeStamp expires;          //时间到达的时间戳
    TimeoutCallBack cb;         //时间到达时的回调函数
    bool cyclicity;             //周期性   用于周期性事件
    int period;                 //周期间隔
    TimerNode(int id, TimeStamp expires, const TimeoutCallBack cb, bool cyclicity = false, int period = 0):id(id), expires(expires), cb(cb), cyclicity(cyclicity), period(period){}
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

        void add(int id, int timeOut, const TimeoutCallBack& cb, bool cyclicity = false, int period = 0);   //添加新节点或修改已有节点

        void doWork(int id);                                        //删除指定id结点，并触发回调函数

        void clear();                                               //清空定时器

        void tick();                                                //执行超时结点的回调函数并清除该节点

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