/**
 * @file httpconn.cpp
 * @author Fansure Grin
 * @date 2024-03-30
 * @brief source file for http connection
*/
#include <cstring>
#include <cassert>
#include <unistd.h>
#include "httpconn.h"
#include "../log/log.h"


std::string HttpConn::src_dir;
bool HttpConn::is_ET;
std::atomic<int> HttpConn::user_count;

HttpConn::HttpConn(): fd(-1), is_close(true), iov_cnt(0) {
    bzero(ip, sizeof(ip));
    bzero(&addr, sizeof(addr));
}

HttpConn::~HttpConn() {
    close_conn();
}

void HttpConn::init(int sock_fd, const sockaddr_in &addr_) {
    assert(sock_fd > 0);
    fd = sock_fd;
    addr = addr_;
    ++user_count;
    write_buf.retrieve_all();
    read_buf.retrieve_all();
    is_close = false;
    LOG_INFO("<client %d, %s:%d> connected! Usercount: %d", fd, get_ip(),
        get_port(), user_count.load());
}

void HttpConn::close_conn() {
    response.unmap_file();
    if (!is_close) {
        is_close = true;
        --user_count;
        close(fd);
        LOG_INFO("<client %d, %s:%d> quited! Usercount: %d", fd, get_ip(),
            get_port(), user_count.load());
    }
}

ssize_t HttpConn::read(int *save_errno) {
    ssize_t len = -1, total_len = 0;
    do {
        len = read_buf.read_fd(fd, save_errno);
        if (len < 0) {
            return len;
        } else if (len == 0) {
            break;
        }
        total_len += len;
    } while (is_ET);
    return total_len;
}

ssize_t HttpConn::write(int *save_errno) {
    ssize_t len = -1, total_len = 0;
    do {
        // it is not an error for a successful call to transfer fewer bytes 
        // than requested
        len = writev(fd, iov, iov_cnt);
        if (len < 0) {
            *save_errno = errno;
            return len;
        }
        total_len += len;
        if (len == 0 && iov[0].iov_len + iov[1].iov_len == 0) {
            break;
        } else if (static_cast<size_t>(len) > iov[0].iov_len) {
            // 考虑iov[1]没有完全写入的情况
            iov[1].iov_base = (char *)iov[1].iov_base + (len - iov[0].iov_len);
            iov[1].iov_len -= (len - iov[0].iov_len);
            if (iov[0].iov_len) {
                write_buf.retrieve_all();
                iov[0].iov_len = 0;
            }
        } else {
            // 考虑iov[0]没有完全写入的情况
            iov[0].iov_base = (char *)iov[0].iov_base + len;
            iov[0].iov_len -= len;
            write_buf.retrieve(len);
        }
    } while (is_ET || to_write_bytes() > 10240);

    return total_len;
}

bool HttpConn::process() {
    request.init();
    if (read_buf.readable_bytes() <= 0) {
        return false;
    }
    if (request.parse(read_buf)) {
        response.init(src_dir, request.get_path(), request.is_keep_alive(), 200);
    } else {
        response.init(src_dir, std::string(), false, 400);
    }

    // 响应的状态行、头部和响应体
    response.make_response(write_buf);
    iov[0].iov_base = const_cast<char*>(write_buf.peek());
    iov[0].iov_len = write_buf.readable_bytes();
    iov_cnt = 1;
    // 响应要发送的文件
    if (response.filelen() > 0 && response.get_file()) {
        iov[1].iov_base = response.get_file();
        iov[1].iov_len = response.filelen();
        iov_cnt = 2;
    }
    LOG_DEBUG("response bytes: %d, file bytes: %d", to_write_bytes(),
        response.filelen());
    return true;
}

int HttpConn::get_fd() const {
    return fd;
}

int HttpConn::get_port() const {
    return ntohs(addr.sin_port);
}

const char* HttpConn::get_ip() {
    inet_ntop(AF_INET, &addr.sin_addr, ip, sizeof(ip));
    return ip;
}

sockaddr_in HttpConn::get_addr() const {
    return addr;
}