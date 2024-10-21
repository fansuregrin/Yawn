/**
 * @file epoller.h
 * @author Fansure Grin
 * @date 2024-03-24
 * @brief header file for epoller
*/
#ifndef EPOLLER_H
#define EPOLLER_H

#include <unistd.h>     // close
#include <cassert>      // assert
#include <cstring>      // strerror
#include <sys/epoll.h>  // epoll_create, epoll_wait, epoll_ctl
#include <vector>
#include "../log/log.h"


class Epoller {
public:
    /**
     * @brief Epoller 构造函数
     * @param max_evnent_number 最大被监听的文件描述符数量，默认为 `1024`
    */
    explicit Epoller(int num_fds = 1024);
    
    /**
     * @brief Epoller 析构函数
    */
    ~Epoller();

    /**
     * @brief 为指定的文件描述符注册事件
     * @param fd 被注册事件的目标文件描述符
     * @param events 被注册的事件
     * @return 是否注册成功
    */
    bool add_fd(int fd, uint32_t events);
    
    /**
     * @brief 从 epoll 内核事件表中修改 fd 上的注册事件
     * @param fd 被修改事件的目标文件描述符
     * @param events 新的事件
     * @return 是否修改成功
    */
    bool mod_fd(int fd, uint32_t events);
    
    /**
     * @brief 从 epoll 内核事件表中删除 fd 上的所有注册事件
     * @param fd 目标文件描述符
     * @return 是否删除成功
    */
    bool del_fd(int fd);
    
    /**
     * @brief 等待就绪事件
     * @param timeout 超时时间，单位为毫秒(milliseconds)
     * @return 有就绪事件的文件描述符的数量
    */
    int wait(int timeout);

    /**
     * @brief 获取有事件发生的文件描述符
     * @param idx 索引
     * @return 文件描述符
    */
    int get_event_fd(size_t idx) const;
    
    /**
     * @brief 获取事件
     * @param idx 索引
     * @return 事件
    */
    uint32_t get_events(size_t idx) const;

private:
    // a file descriptor referring to the new epoll instance
    int m_epoll_fd;

    // 存放 `epoll_event` 的数组
    std::vector<struct epoll_event> m_epoll_events;
};

#endif // EPOLLER_H