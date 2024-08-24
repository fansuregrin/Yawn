/**
 * @file httpconn.h
 * @author Fansure Grin
 * @date 2024-03-30
 * @brief header file for http connection
*/
#ifndef HTTPCONN_H
#define HTTPCONN_H

#include <atomic>
#include <arpa/inet.h>
#include <sys/uio.h>
#include "../buffer/buffer.h"
#include "httprequest.h"
#include "httpresponse.h"


class HttpConn {
public:
    HttpConn();
    ~HttpConn();

    void init(int sock_fd, const sockaddr_in &addr_);
    void close_conn();
    ssize_t read(int *save_errno);
    ssize_t write(int *save_errno);
    bool process();
    int get_fd() const;
    int get_port() const;
    const char* get_ip();
    sockaddr_in get_addr() const;

    bool is_keep_alive() const {
        return request.is_keep_alive();
    }

    int to_write_bytes() {
        return iov[0].iov_len + iov[1].iov_len;
    }

    static std::string src_dir;
    static bool is_ET;
    static std::atomic<int> conn_count;
private:
    int fd;
    struct sockaddr_in addr;
    char ip[32];
    bool is_close;
    int iov_cnt;
    struct iovec iov[2];
    Buffer read_buf;
    Buffer write_buf;
    HttpRequest request;
    HttpResponse response;
};

#endif // HTTPCONN_H