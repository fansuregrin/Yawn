/**
 * @file epoller.cpp
 * @author Fansure Grin
 * @date 2024-03-24
 * @brief source file for epoller
*/
#include <unistd.h>     // close
#include <cassert>      // assert
#include "epoller.h"


/**
 * @brief 初始化一个 Epoller 实例
 * @param max_evnent_number 最大事件数量，默认为`1024`
*/
Epoller::Epoller(int max_event_number)
: epoll_fd(epoll_create(512)), events(max_event_number) {
    assert(epoll_fd >= 0 && events.size() > 0);
}

/**
 * @brief 销毁一个 Epoller 实例
*/
Epoller::~Epoller() {
    close(epoll_fd);
}

/**
 * @brief 为指定的文件描述符注册事件
 * @param fd 被注册事件的目标文件描述符
 * @param events 被注册的事件
 * @return 是否注册成功
*/
bool Epoller::add_fd(int fd, uint32_t events) {
    if (fd < 0) return false;
    epoll_event event;
    event.data.fd = fd;
    event.events = events;
    return epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &event) == 0;
}

/**
 * @brief 从 epollfd 标识的 epoll 内核事件表中修改 fd 上的注册事件
 * @param fd 被修改事件的目标文件描述符
 * @param events 新的事件
 * @return 是否修改成功
*/
bool Epoller::mod_fd(int fd, uint32_t events) {
    if (fd < 0) return false;
    epoll_event event;
    event.data.fd = fd;
    event.events = events;
    return epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd, &event) == 0;
}

/**
 * @brief 从 epollfd 标识的 epoll 内核事件表中删除 fd 上的所有注册事件，并关闭 fd
 * @param fd 目标文件描述符
 * @return 是否删除成功
*/
bool Epoller::del_fd(int fd) {
    if (fd < 0) return false;
    // Since Linux 2.6.9, event can be specified as NULL when using EPOLL_CTL_DEL. 
    // However, applications that need to be portable to kernels before 
    // Linux 2.6.9 should specify a non-null pointer in event.
    epoll_event event = {0};
    return epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, &event) == 0;
}

/**
 * @brief 等待 I/O 事件
 * @param timeout 超时时间，单位为毫秒(milliseconds)
 * @return 为请求的 I/O 准备好的文件描述符的数量
*/
int Epoller::wait(int timeout) {
    return epoll_wait(epoll_fd, &events[0], static_cast<int>(events.size()), timeout);
}

/**
 * @brief 获取有事件发生的文件描述符
 * @param idx 索引
 * @return 文件描述符
*/
int Epoller::get_event_fd(size_t idx) const {
    assert(idx >= 0 && idx < events.size());
    return events[idx].data.fd;
}

/**
 * @brief 获取事件
 * @param idx 索引
 * @return 事件
*/
uint32_t Epoller::get_events(size_t idx) const {
    assert(idx >= 0 && idx < events.size());
    return events[idx].events;
}