/**
 * @file blocking_deque.hpp
 * @author Fansure Grin
 * @date 2024-03-25
 * @brief blocking deque
*/
#ifndef BLOCKING_DEQUE_HPP
#define BLOCKING_DEQUE_HPP

#include <deque>
#include <condition_variable>
#include <mutex>

template <typename T>
class BlockingDeque {
private:
    using lock_guard = std::lock_guard<std::mutex>;
    using unique_lock = std::unique_lock<std::mutex>;
public:
    using size_type = typename std::deque<T>::size_type;
    using value_type = T;
    using reference = value_type &;
    using const_reference = const value_type &;

    BlockingDeque(size_type max_cap = 1000);
    ~BlockingDeque();

    value_type front();
    value_type back();

    bool empty() noexcept;
    bool full() noexcept;
    size_type size() noexcept;
    size_type capacity() noexcept;

    void clear() noexcept;
    void push_front(const T &ele);
    void push_back(const T &ele);
    bool pop(T &ele);
    bool pop(T &ele, int timeout);

    void flush();
    void close();
private:
    std::deque<T> deq;
    std::mutex mtx;
    size_type cap;
    std::condition_variable producer;
    std::condition_variable consumer;
    bool is_close;
};

template <typename T>
inline BlockingDeque<T>::BlockingDeque(size_type max_cap)
: cap(max_cap) {
    is_close = false;
}

template <typename T>
BlockingDeque<T>::~BlockingDeque() {
    close();
}

template <typename T>
typename BlockingDeque<T>::value_type BlockingDeque<T>::front() {
    lock_guard lck(mtx);
    return deq.front();
}

template <typename T>
typename BlockingDeque<T>::value_type BlockingDeque<T>::back() {
    lock_guard lck(mtx);
    return deq.front();
}

template <typename T>
bool BlockingDeque<T>::empty() noexcept {
    lock_guard lck(mtx);
    return deq.empty();
}

template <typename T>
bool BlockingDeque<T>::full() noexcept {
    lock_guard lck(mtx);
    return deq.size() >= cap;
}

template <typename T>
typename BlockingDeque<T>::size_type
BlockingDeque<T>::size() noexcept {
    lock_guard lck(mtx);
    return deq.size();
}

template <typename T>
typename BlockingDeque<T>::size_type
BlockingDeque<T>::capacity() noexcept {
    lock_guard lck(mtx);
    return cap;
}

template <typename T>
void BlockingDeque<T>::clear() noexcept {
    lock_guard lck(mtx);
    deq.clear();
}

template <typename T>
void BlockingDeque<T>::push_front(const T &ele) {
    unique_lock lck(mtx);
    while (deq.size() >= cap) {
        producer.wait(lck);
    }
    deq.push_front(ele);
    consumer.notify_one();
}

template <typename T>
void BlockingDeque<T>::push_back(const T &ele) {
    unique_lock lck(mtx);
    while (deq.size() >= cap) {
        producer.wait(lck);
    }
    deq.push_back(ele);
    consumer.notify_one();
}

template <typename T>
bool BlockingDeque<T>::pop(T &ele) {
    unique_lock lck(mtx);
    while (deq.empty()) {
        consumer.wait(lck);
        if (is_close) {
            return false;
        }
    }
    ele = deq.front();
    deq.pop_front();
    producer.notify_one();
    return true;
}

template <typename T>
bool BlockingDeque<T>::pop(T &ele, int timeout) {
    unique_lock lck(mtx);
    while (deq.empty()) {
        if (consumer.wait_for(lck, std::chrono::seconds(timeout)) 
            == std::cv_status::timeout) {
            return false;
        }
        if (is_close) {
            return false;
        }
    }
    ele = deq.front();
    deq.pop_front();
    producer.notify_one();
    return true;
}

template <typename T>
void BlockingDeque<T>::flush() {
    consumer.notify_one();
}

template <typename T>
void BlockingDeque<T>::close() {
    {
        lock_guard lck(mtx);
        deq.clear();
        is_close = true;
    }
    producer.notify_all();
    consumer.notify_all();
}

#endif // BLOCKING_DEQUE_HPP