/**
 * @file epoller.cpp
 * @author Fansure Grin
 * @date 2024-03-24
 * @brief source file for epoller
*/
#include "epoller.h"


Epoller::Epoller(int num_fds)
: m_epoll_fd(epoll_create(512)), m_epoll_events(num_fds) {
    if (m_epoll_fd < 0) {
        LOG_ERROR("epoll_create failed: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }
}

Epoller::~Epoller() {
    close(m_epoll_fd);
}

bool Epoller::add_fd(int fd, uint32_t events) {
    if (fd < 0) return false;
    epoll_event event;
    event.data.fd = fd;
    event.events = events;
    return epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, fd, &event) == 0;
}

bool Epoller::mod_fd(int fd, uint32_t events) {
    if (fd < 0) return false;
    epoll_event event;
    event.data.fd = fd;
    event.events = events;
    return epoll_ctl(m_epoll_fd, EPOLL_CTL_MOD, fd, &event) == 0;
}

bool Epoller::del_fd(int fd) {
    if (fd < 0) return false;
    // Since Linux 2.6.9, event can be specified as NULL when using EPOLL_CTL_DEL. 
    // However, applications that need to be portable to kernels before 
    // Linux 2.6.9 should specify a non-null pointer in event.
    epoll_event event = {0};
    return epoll_ctl(m_epoll_fd, EPOLL_CTL_DEL, fd, &event) == 0;
}

int Epoller::wait(int timeout) {
    return epoll_wait(m_epoll_fd, &m_epoll_events[0], 
        static_cast<int>(m_epoll_events.size()), timeout);
}

int Epoller::get_event_fd(size_t idx) const {
    assert(idx >= 0 && idx < m_epoll_events.size());
    return m_epoll_events[idx].data.fd;
}

uint32_t Epoller::get_events(size_t idx) const {
    assert(idx >= 0 && idx < m_epoll_events.size());
    return m_epoll_events[idx].events;
}