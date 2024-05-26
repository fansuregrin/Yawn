/**
 * @file heap_timer.cpp
 * @author Fansure Grin
 * @date 2024-03-27
 * @brief source file for timer and time_heap
*/
#include <cassert>
#include "heap_timer.h"


void TimeHeap::add(int id, int timeout, const TimeoutCallback &cb_) {
    assert(id >= 0);
    size_type idx = 0;
    if (ref.count(id) == 0) {
        // 新的节点，将其插入堆尾再上滤
        idx = heap.size();
        ref[id] = idx;
        heap.emplace_back(id, Clock::now()+MSec(timeout), cb_);
        sift_up(idx);
    } else {
        // 堆中原来就存在此节点
        idx = ref[id];
        heap[idx].expire = Clock::now() + MSec(timeout);
        heap[idx].cb = cb_;
        if (!sift_down(idx, heap.size())) {
            sift_up(idx);
        }
    }
}

void TimeHeap::pop() {
    assert(!heap.empty());
    del(0);
}

void TimeHeap::adjust(int id, int timeout) {
    if(empty() || ref.count(id) <= 0) {
        return;
    }
    heap[ref[id]].expire = Clock::now() + MSec(timeout);
    sift_down(ref[id], heap.size());
}

void TimeHeap::clear() {
    heap.clear();
    ref.clear();
}

void TimeHeap::tick() {
    if (heap.empty()) {
        return;
    }
    while (!heap.empty()) {
        Timer tm = heap.front();
        if (std::chrono::duration_cast<MSec>(tm.expire-Clock::now()).count() > 0) {
            break;
        }
        tm.cb();
        pop();
    }
}

int TimeHeap::get_next_tick() {
    tick();
    int res = -1;
    if (!heap.empty()) {
        res = std::chrono::duration_cast<MSec>(heap.front().expire - Clock::now())
                .count();
        // 如果下一个要处理的定时任务已经超时，则需要立即处理
        // 所以等待的时间设置为 0
        if (res < 0) res = 0;
    }
    // 当定时任务为空时，无需处理定时任务，可以安心地等待
    // 等待的时间设置为 -1
    return res;
}

void TimeHeap::do_work(int id) {
    if (heap.empty() || ref.count(id)==0) {
        return;
    }
    size_type idx = ref[id];
    Timer tm = heap[idx];
    tm.cb();
    del(idx);
}

void TimeHeap::sift_up(size_type idx) {
    assert(idx >= 0 && idx < heap.size());
    size_type parent = 0;
    while (idx > 0) {
        parent = (idx - 1) / 2;
        if (heap[parent] < heap[idx]) {
            break;
        }
        swap_node(parent, idx);
        idx = parent;
    }
}

bool TimeHeap::sift_down(size_type idx, size_type n) {
    assert(idx >= 0 && idx < n);
    assert(n >= 0 && n <= heap.size());
    size_type hole = idx;
    size_type child = 2 * hole + 1;  // 左孩子结点
    while (child < n) {
        if (child+1<n && heap[child+1] < heap[child]) {
            ++child;
        }
        if (heap[hole] < heap[child]) {
            break;
        }
        swap_node(hole, child);
        hole = child;
        child = 2 * hole + 1;
    }
    return hole > idx;
}

void TimeHeap::swap_node(size_type t1, size_type t2) {
    assert(t1 >= 0 && t1 < heap.size());
    assert(t2 >= 0 && t2 < heap.size());
    std::swap(heap[t1], heap[t2]);
    ref[heap[t1].id] = t1;
    ref[heap[t2].id] = t2;
}

void TimeHeap::del(size_type idx) {
    assert(!heap.empty() && idx >= 0 && idx < heap.size());
    size_type last = heap.size() - 1;
    assert(idx <= last);
    if (idx < last) {
        swap_node(idx, last);
        if (!sift_down(idx, last)) {
            sift_up(idx);
        }
    }
    ref.erase(heap.back().id);
    heap.pop_back();
}