/**
 * @file httprequest.h
 * @author Fansure Grin
 * @date 2024-03-28
 * @brief header file for http-request
*/
#ifndef HTTPREQUEST_H
#define HTTPREQUEST_H

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <regex>
#include "../buffer/buffer.h"


class HttpRequest {
public:

    enum PARSE_STATE {
        REQUEST_LINE,
        HEADERS,
        BODY,
        FINISH
    };

    enum HTTP_CODE {
        NO_REQUEST = 0,
        GET_REQUEST,
        BAD_REQUEST,
        NO_RESOURCE,
        FORBIDDEN_REQUEST,
        FILE_REQUEST,
        INTERNEL_ERROR,
        CLOSED_CONNECTION
    };

    HttpRequest() { init(); }
    ~HttpRequest() = default;

    void init();
    bool parse(Buffer &buf);

    bool is_keep_alive() const;
    std::string get_path() const;
    std::string& get_path();
    std::string get_method() const;
    std::string get_version() const;
    std::string get_post(const std::string &key) const;
    std::string get_post(const char *key) const;
private:
    bool parse_requestline(const std::string &line);
    void parse_header(const std::string &line);
    void parse_body(const std::string &line);

    void parse_post();
    void parse_form_urlencoded();
    void parse_path();
    bool verify_user(const std::string &username, const std::string &passwd,
            bool is_login);

    PARSE_STATE state;    // 请求的解析状态
    std::string method;   // 请求方法
    std::string path;     // 要访问的资源路径
    std::string version;  // HTTP 版本
    std::string body;     // 请求的消息体
    std::unordered_map<std::string,std::string> headers;  // 请求头部
    std::unordered_map<std::string,std::string> post;  // POST请求
    
    static std::unordered_set<std::string> DEFAULT_HTML;
    static std::unordered_map<std::string,int> DEFAULT_HTML_TAG;
    static const std::regex re_requestline;
    static const std::regex re_header;
    static int convert_hex(char ch);
    static std::string str_lower(std::string str);
};

#endif