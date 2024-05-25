/**
 * @file threadpool.hpp
 * @author Fansure Grin
 * @date 2024-03-28
 * @brief theadpool
*/
#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <mutex>
#include <condition_variable>
#include <queue>
#include <functional>
#include <memory>
#include <thread>
#include <cassert>

class ThreadPool {
public:
    explicit ThreadPool(int thread_count_=8)
    : pool(std::make_shared<Pool>()), thread_count(thread_count_) {
        assert(thread_count > 0);
        for (int i=0; i<thread_count; ++i) {
            std::thread([pool_ = pool] {
                std::unique_lock<std::mutex> lck(pool_->mtx);
                while (true) {
                    if (!pool_->tasks.empty()) {
                        auto task = std::move(pool_->tasks.front());
                        pool_->tasks.pop();
                        lck.unlock();
                        task();
                        lck.lock();
                    } else if (pool_->is_closed) {
                        break;
                    } else {
                        pool_->cond.wait(lck);
                    }
                }
            }).detach();
        }
    }

    ThreadPool() = default;
    ThreadPool(ThreadPool &&) = default;
    
    ~ThreadPool() {
        if (static_cast<bool>(pool)) {
            {
                std::lock_guard<std::mutex> lck(pool->mtx);
                pool->is_closed = true;
            }
            pool->cond.notify_all();
        }
    }

    template <typename F>
    void add_task(F &&task) {
        {
            std::lock_guard<std::mutex> lck(pool->mtx);
            pool->tasks.emplace(std::forward<F>(task));
        }
        pool->cond.notify_one();
    }

    size_t get_thread_count() { return thread_count; }
private:
    struct Pool {
        std::mutex mtx;
        std::condition_variable cond;
        bool is_closed;
        std::queue<std::function<void()>> tasks;
    };
    std::shared_ptr<Pool> pool;
    size_t thread_count;
};

#endif