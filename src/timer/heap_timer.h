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

using TimeStemp = std::chrono::high_resolution_clock::time_point;
using Clock = std::chrono::high_resolution_clock;
using MSec = std::chrono::milliseconds;
using TimeoutCallback = std::function<void()>;

struct Timer {
    int id;
    TimeStemp expire;
    TimeoutCallback cb;

    Timer(int id_, TimeStemp expire_, const TimeoutCallback &cb_)
        : id(id_), expire(expire_), cb(cb_) {}
    
    bool operator<(const Timer &rhs) {
        return expire < rhs.expire;
    }
};


class TimeHeap {
public:
    using size_type = std::vector<Timer>::size_type;

    TimeHeap() { heap.reserve(64); }
    ~TimeHeap() { clear(); }

    void add(int id, int timeout, const TimeoutCallback &cb_);
    void pop();
    void adjust(int id, int timeout);
    void clear();

    void tick();
    int get_next_tick();
    void do_work(int id);
private:
    void sift_up(size_type idx);
    bool sift_down(size_type idx, size_type n);
    void swap_node(size_type t1, size_type t2);
    void del(size_type idx);

    std::vector<Timer> heap;
    std::unordered_map<int, size_type> ref;
};

#endif // HEAP_TIMER_H