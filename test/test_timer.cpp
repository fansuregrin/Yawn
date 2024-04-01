/**
 * @file test_timer.cpp
 * @author Fansure Grin
 * @date 2024-03-26
 * @brief timer模块的测试程序
*/
#include <iostream>
#include <random>
#include "../src/timer/heap_timer.h"

void do_something() {
    std::cout << "Timeover! I must do something!" << std::endl;
}

void test_timer() {
    TimeHeap t_heap;
    std::uniform_int_distribution<int> u(10, 10000);
    std::default_random_engine e;
    for (int i=0; i<10; ++i) {
        t_heap.add(i, u(e), do_something);
    }
    t_heap.get_next_tick();
}