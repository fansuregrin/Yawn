/**
 * @file epoller.h
 * @author Fansure Grin
 * @date 2024-03-24
 * @brief header file for epoller
*/
#ifndef EPOLLER_H
#define EPOLLER_H

#include <sys/epoll.h>
#include <vector>


/**
 * @brief 
*/
class Epoller {
public:
    explicit Epoller(int max_event_number = 1024);
    ~Epoller();
public:
    bool add_fd(int fd, uint32_t events);
    bool mod_fd(int fd, uint32_t events);
    bool del_fd(int fd);
    int wait(int timeout);
    int get_event_fd(size_t idx) const;
    uint32_t get_events(size_t idx) const;
private:
    // a file descriptor referring to the new epoll instance
    int epoll_fd;
    // 存放 `epoll_event` 的容器
    std::vector<struct epoll_event> events;
};

#endif // EPOLLER_H