/**
 * @file test_threadpool.cpp
 * @author Fansure Grin
 * @date 2024-04-02
 * @brief 线程池模块的测试程序
*/
#include <iostream>
#include <thread>
#include <functional>
#include <vector>
#include <chrono>
#include <mutex>
#include "../src/pool/threadpool.hpp"

std::mutex mtx;

struct Task {
    Task(int id_): id(id_), finished(false) {}

    int id;
    bool finished;
};

void do_task(Task *task) {
    task->finished = true;
    {
        std::unique_lock<std::mutex> lck(mtx);
        std::cout << "Task " << task->id << " executed by thread "
            << std::this_thread::get_id() << std::endl;
    }
}

void test_threadpool(int thread_num, int task_num) {
    {
        std::unique_lock<std::mutex> lck(mtx);
        std::cout << "in main thread [" << std::this_thread::get_id()
            << "]: I created a thread pool which has " << thread_num
            << " thread(s) and submited " << task_num << " tasks!"
            << std::endl;
    }
    ThreadPool pool(thread_num);
    std::vector<Task> tasks;
    for (int i=0; i<task_num; ++i) {
        tasks.emplace_back(i);
    }
    for (int i=0; i<task_num; ++i) {
        pool.add_task(std::bind(do_task, &tasks[i]));
    }
    std::this_thread::sleep_for(std::chrono::seconds(10));

    for (int i=0; i<task_num; ++i) {
        assert(tasks[i].finished == true);
    }
}