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


Buffer::Buffer(size_type size): buff(size), read_pos(0), write_pos(0) {}

Buffer::size_type Buffer::readable_bytes() const {
    return write_pos - read_pos;
}

Buffer::size_type Buffer::writable_bytes() const {
    return buff.size() - write_pos;
}

Buffer::size_type Buffer::prependable_bytes() const {
    return read_pos;
}

const char * Buffer::peek() const {
    return begin() + read_pos;
}

void Buffer::retrieve(size_type len) {
    if (len < readable_bytes()) {
        read_pos += len;
    } else {
        retrieve_all();
    }
}

void Buffer::retrieve_until(const char * end) {
    if (end < peek()) return;
    end = std::min(end, const_cast<const char*>(begin_write()));
    retrieve(end - peek());
}

void Buffer::retrieve_all() {
    read_pos = 0;
    write_pos = 0;
}

std::string Buffer::retrieve_as_str(size_type len) {
    if (len >= readable_bytes()) {
        len = readable_bytes();
    }
    std::string str(peek(), len);
    retrieve(len);
    return str;
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

char * Buffer::begin() {
    return &*buff.begin();
}

const char * Buffer::begin() const {
    return &*buff.begin();
}

void Buffer::make_space(size_type sz) {
    if (prependable_bytes() + writable_bytes() < sz) {
        // “前置的空闲空间”加上“可写入的空间”已经不够写入 `sz` 个字节
        // 需要扩容（如果大小没有超过 vector<char> 的 capcity，则不会申请内存空间）
        buff.resize(read_pos + sz);
    } else {
        // “前置的空闲空间”加上“可写入的空间”已经足够写入 `sz` 个字节
        // 此时无需扩容，只需要将可读数据移动到缓冲区的起始位置，覆盖“前置的空闲空间”即可
        size_type readable_len = readable_bytes();
        std::copy(begin()+read_pos, begin()+write_pos, begin());
        read_pos = 0;
        write_pos = read_pos + readable_len;
        assert(readable_len == readable_bytes());
    }
}