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
    friend class HttpConn;

    HttpRequest() { init(); }
    ~HttpRequest() = default;

    void init();

    std::string get_path() const;
    std::string& get_path();
    std::string get_method() const;
    std::string get_version() const;
    std::string get_post(const std::string &key) const;
    std::string get_header(const std::string &key) const;
private:
    std::string method;   // 请求方法
    std::string request_uri;
    std::string path;     // 要访问的资源路径
    std::unordered_map<std::string,std::string> query;
    std::string version;  // HTTP 版本
    std::string body;     // 请求的消息体
    std::unordered_map<std::string,std::string> headers;  // 请求头部
    std::unordered_map<std::string,std::string> post;  // POST请求
};

#endif