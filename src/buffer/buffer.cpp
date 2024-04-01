/**
 * @file buffer.cpp
 * @author Fansure Grin
 * @date 2024-03-24
 * @brief source file for buffer
*/
#include <sys/uio.h>  // readv
#include <unistd.h>   // write
#include <cerrno>     // errno
#include <cassert>    // assert
#include <algorithm>  // copy
#include "buffer.h"


/**
 * @brief 初始化缓冲区
 * @param size 缓冲区初始大小
*/
Buffer::Buffer(size_type size): buff(size), read_pos(0), write_pos(0) {}

/**
 * @brief 缓冲区中可读的字节数
 * @return 字节数目
*/
Buffer::size_type Buffer::readable_bytes() const {
    return write_pos - read_pos;
}

/**
 * @brief 缓冲区中可写的字节数
 * @return 字节数目
*/
Buffer::size_type Buffer::writable_bytes() const {
    return buff.size() - write_pos;
}

/**
 * @brief 缓冲区中前置的空闲字节数目
 * @return 字节数目
*/
Buffer::size_type Buffer::prependable_bytes() const {
    return read_pos;
}

const char * Buffer::peek() const {
    return begin() + read_pos;
}

void Buffer::retrieve(size_type len) {
    assert(len <= readable_bytes());
    if (len < readable_bytes()) {
        read_pos += len;
    } else {
        retrieve_all();
    }
}

void Buffer::retrieve_until(const char * end) {
    assert(end >= peek());
    retrieve(end - peek());
}

void Buffer::retrieve_all() {
    read_pos = 0;
    write_pos = 0;
}

std::string Buffer::retrieve_all_as_str() {
    std::string str(peek(), readable_bytes());
    retrieve_all();
    return str;
}

char * Buffer::begin_write() {
    return begin() + write_pos;
}

const char * Buffer::begin_write() const {
    return begin() + write_pos;
}

const char * Buffer::begin_write_const() {
    return begin() + write_pos;
}

void Buffer::has_written(size_type sz) {
    write_pos += sz;
}

void Buffer::ensure_writable(size_type sz) {
    if (writable_bytes() < sz) {
        make_space(sz);
    }
    assert(writable_bytes() >= sz);
}

void Buffer::append(const char *p, size_type sz) {
    if (!p) return;
    ensure_writable(sz);
    std::copy(p, p+sz, begin_write());
    write_pos += sz;
}

void Buffer::append(const std::string &str) {
    append(str.data(), str.size());
}

void Buffer::append(const void * p, size_type sz) {
    if (!p) return;
    append(static_cast<const char *>(p), sz);
}

void Buffer::append(const Buffer &other) {
    append(other.peek(), other.readable_bytes());
}

ssize_t Buffer::read_fd(int fd, int * saved_errno) {
    char extra_buf[65536];
    struct iovec iov[2];
    size_type writable = writable_bytes();
    iov[0].iov_base = begin_write();
    iov[0].iov_len = writable;
    iov[1].iov_base = extra_buf;
    iov[1].iov_len = sizeof(extra_buf);
    int len = readv(fd, iov, 2);
    if (len < 0) {
        *saved_errno = errno;
    } else if (static_cast<size_type>(len) <= writable) {
        write_pos += len;
    } else {
        write_pos = buff.size();
        append(extra_buf, len-writable);
    }
    return len;
}

ssize_t Buffer::write_fd(int fd, int * saved_errno) {
    int len = write(fd, peek(), readable_bytes());
    if (len < 0) {
        *saved_errno = errno;
        return len;
    }
    read_pos += len;
    return len;
}

/**
 * @brief 获取缓冲区的起始地址
 * @return 起始地址
*/
char * Buffer::begin() {
    return &*buff.begin();
}

/**
 * @brief 获取缓冲区的起始地址 (const 版本)
 * @return 起始地址
*/
const char * Buffer::begin() const {
    return &*buff.begin();
}

void Buffer::make_space(size_type sz) {
    if (prependable_bytes() + writable_bytes() < sz) {
        buff.resize(writable_bytes() + sz);
    } else {
        size_type readable_len = readable_bytes();
        std::copy(begin()+read_pos, begin()+write_pos, begin());
        read_pos = 0;
        write_pos = read_pos + readable_len;
        assert(readable_len == readable_bytes());
    }
}