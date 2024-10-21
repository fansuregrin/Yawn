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
    enum PARSE_STATE {
        REQUEST_LINE,  // 解析请求行
        HEADERS,       // 解析请求头
        BODY,          // 解析请求体
        FINISH         // 解析完成
    };

    enum PARSE_RESULT {
        OK = 0,      // 解析成功（合法且完整的 HTTP 请求）
        ERROR,       // 解析失败（非法的 HTTP 请求）
        EMPTY,       // 解析失败（空的HTTP请求）
        NOT_FINISH   // 解析失败（不完整的 HTTP 请求）
    };

    HttpConn();
    ~HttpConn();

    HttpConn::PARSE_RESULT parse(Buffer &buf);
    bool parse_requestline(const std::string &line);
    bool parse_uri(const std::string &uri);
    bool parse_header(const std::string &line);
    void parse_body(const std::string &content);
    void parse_post();
    void parse_form_urlencoded();

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
    PARSE_STATE state;    // 请求的解析状态
    Buffer read_buf;
    Buffer write_buf;
    HttpRequest request;
    HttpResponse response;

    static const std::regex re_requestline;
    static const std::regex re_header;
};

#endif // HTTPCONN_H