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
#include <sys/stat.h>
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
    void make_response();

    void init(int sock_fd, const sockaddr_in &addr_);
    void close_conn();
    ssize_t read(int *save_errno);
    ssize_t write(int *save_errno);
    bool process();
    int get_fd() const;
    int get_port() const;
    const char* get_ip();
    sockaddr_in get_addr() const;

    bool is_keep_alive() const;

    int to_write_bytes() {
        return iov[0].iov_len + iov[1].iov_len;
    }

    static std::string src_dir;
    static bool is_ET;
    static std::atomic<int> conn_count;
private:
    bool parse_requestline(const std::string &line);
    bool parse_uri(const std::string &uri);
    bool parse_header(const std::string &line);
    void parse_body(const std::string &content);
    void parse_post();
    void parse_form_urlencoded();

    void set_status_line();
    void set_headers();
    bool check_resource_and_map(const std::string &fp);
    bool map_file(const std::string &fp);
    std::string get_file_type(const std::string &fp);
    void set_err_content();
    std::string get_default_err_content();
    void unmap_file();
    const char * get_mm_file() const;
    decltype(stat::st_size) get_mm_file_len() const;

    int fd;
    struct sockaddr_in addr;
    char ip[32];
    bool is_close;
    int iov_cnt;
    struct iovec iov[2];
    PARSE_STATE state;    // 请求的解析状态
    Buffer read_buf;
    Buffer write_buf;
    char * mm_file;              // 文件映射到内存中的地址
    struct stat mm_file_stat;    // 被映射文件的状态信息
    HttpRequest request;
    HttpResponse response;

    static const std::regex re_requestline;
    static const std::regex re_header;
    // 文件扩展名到媒体类型的映射表
    static const std::unordered_map<std::string,std::string> SUFFIX_TYPE;
    // 状态码到状态信息的映射表
    static const std::unordered_map<int,std::string> STATUS_TEXT;
    // 状态码到响应资源文件的映射表
    static const std::unordered_map<int,std::string> CODE_PATH;
};

#endif // HTTPCONN_H