/**
 * @file heap_timer.h
 * @author Fansure Grin
 * @date 2024-03-27
 * @brief header file for timer and time_heap
*/
#ifndef HEAP_TIMER_H
#define HEAP_TIMER_H

#include <chrono>
#include <functional>
#include <vector>
#include <unordered_map>

using TimePoint = std::chrono::high_resolution_clock::time_point;
using Clock = std::chrono::high_resolution_clock;
using MSec = std::chrono::milliseconds;
using TimeoutCallback = std::function<void()>;

struct Timer {
    int id;                       // 定时器的唯一标识
    TimePoint expire;             // 定时器的过期时间
    TimeoutCallback cb;           // 定时器过期后调用的回调函数

    /**
     * @brief 定时器构造函数
     * @param id_ 定时器的编号
     * @param expire_ 定时器的过期时间
     * @param cb_ 定时器过期后调用的回调函数
     */
    Timer(int id_, TimePoint expire_, const TimeoutCallback &cb_)
        : id(id_), expire(expire_), cb(cb_) {}
    
    bool operator<(const Timer &rhs) {
        return expire < rhs.expire;
    }
};


/**
 * @brief 最小时间堆
 */
class TimeHeap {
public:
    using size_type = std::vector<Timer>::size_type;

    TimeHeap() { heap.reserve(64); }
    ~TimeHeap() { clear(); }

    /**
     * @brief 向堆中添加一个定时器
     * @param id 定时器的唯一标识
     * @param timeout 定时器的超时时间（单位为毫秒）
     * @param cb_ 定时器超时后的回调函数
    */
    void add(int id, int timeout, const TimeoutCallback &cb_);
    
    /**
     * @brief 弹出堆顶的定时器
    */
    void pop();

    /**
     * @brief 给指定定时器延长超时时间
     * @param id 定时器的唯一标识
     * @param timeout 需要延长的时间（单位为毫秒）
     */
    void adjust(int id, int timeout);

    /**
     * @brief 清空堆
    */
    void clear();

    /**
     * @brief 心搏函数，执行超时的定时器的回调函数并将其从堆中移除
     */
    void tick();

    /**
     * @brief 获取下一次调用心搏函数的间隔时间
     * @return 间隔时间（单位为毫秒）
     */
    int get_next_tick();

    void do_work(int id);

    bool empty() { return heap.empty(); }
    size_type size() { return heap.size(); }
private:
    /**
     * @brief 最小堆的上滤操作
     * @param idx 上滤开始位置的索引号
     */
    void sift_up(size_type idx);

    /**
     * @brief 最小堆的下滤操作
     * @param idx 下滤开始位置的索引号
     * @param n 下滤结束位置的索引号
     * @return 是否能下滤
     */
    bool sift_down(size_type idx, size_type n);

    /**
     * @brief 交换堆中的两个结点
     * @param t1 结点 1 的索引号
     * @param t2 结点 2 的索引号
    */
    void swap_node(size_type t1, size_type t2);
    
    /**
     * @brief 删除堆中的结点
     * @param idx 被删除结点的索引号
     */
    void del(size_type idx);

    std::vector<Timer> heap;  // 定时器容器
    // 定时器的唯一标识 到 定时器在堆中的索引号 的映射
    std::unordered_map<int, size_type> ref;
};

#endif // HEAP_TIMER_H