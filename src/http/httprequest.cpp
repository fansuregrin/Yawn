/**
 * @file httprequest.cpp
 * @author Fansure Grin
 * @date 2024-03-28
 * @brief source file for http-request
*/
#include <regex>
#include <algorithm>
#include <mysql/mysql.h>
#include <cstring>
#include "httprequest.h"
#include "../log/log.h"
#include "../pool/sqlconnRAII.hpp"
#include "../pool/sqlconnpool.h"

using std::regex;
using std::regex_match;
using std::smatch;


/**
 * 匹配 HTTP 请求行的正则表达式 
 * Request-Line   = Method SP Request-URI SP HTTP-Version CRLF 
 * HTTP-Version   = "HTTP" "/" 1*DIGIT "." 1*DIGIT
*/
const regex HttpRequest::re_requestline("^([^ ]*) ([^ ]*) HTTP/(\\d+\\.\\d+)$");

/**
 * 匹配消息头部的正则表达式
 * Each header field consists of a name followed by a colon (":") and 
 * the field value. Field names are case-insensitive. The field value MAY 
 * be preceded by any amount of LWS, though a single SP is preferred.
 * 
 * message-header = field-name ":" [ field-value ]
 * field-name     = token
 * field-value    = *( field-content | LWS )
 * field-content  = <the OCTETs making up the field-value
 *                  and consisting of either *TEXT or combinations
 *                  of token, separators, and quoted-string>
*/
const regex HttpRequest::re_header("^([^:]*):[ \t]*(.*)$");

int HttpRequest::convert_hex(char ch) {
    if (ch <= 'F' && ch >= 'A') return ch - 'A' + 10;
    if (ch <= 'f' && ch >= 'a') return ch - 'a' + 10;
    return ch - '0';
}

std::string HttpRequest::str_lower(std::string str) {
    for (auto &ch:str) {
        if (ch >= 'A' && ch <= 'Z') {
            ch += 32;
        }
    }
    return str;
}

void HttpRequest::init() {
    state = REQUEST_LINE;
    method = path = version = body = "";
    headers.clear();
    post.clear();
}

bool HttpRequest::parse(Buffer &buf) {
    const char CRLF[] = "\r\n";
    if (buf.readable_bytes() <= 0) return false;
    while (buf.readable_bytes() && state != FINISH) {
        if (state == BODY) {
            parse_body(buf.retrieve_all_as_str());
            continue;
        }
        const char *line_end = std::search(buf.peek(), buf.begin_write_const(),
            CRLF, CRLF+2);
        std::string line(buf.peek(), line_end);
        switch (state) {
            case REQUEST_LINE: {
                if (!parse_requestline(line)) {
                    return false;
                }
                parse_uri();
                break;
            }
            case HEADERS: {
                parse_header(line);
                // 如果缓冲区可读字节只有两个（即"\r\n"），则直接转到FINISH状态
                // 说明此次请求不携带消息体
                if (buf.readable_bytes() <= 2) {
                    state = FINISH;
                }
                break;
            }
        }
        buf.retrieve_until(line_end+2);
    }
    return true;
}

bool HttpRequest::is_keep_alive() const {
    auto target = headers.find("connection");
    return target != headers.end() && str_lower(target->second) == "keep-alive";
}

std::string HttpRequest::get_path() const {
    return path;
}

std::string& HttpRequest::get_path() {
    return path;
}

std::string HttpRequest::get_method() const {
    return method;
}

std::string HttpRequest::get_version() const {
    return version;
}

std::string HttpRequest::get_post(const std::string &key) const {
    auto target = post.find(key);
    if (target != post.end()) {
        return target->second;
    }
    return "";
}

std::string HttpRequest::get_post(const char *key) const {
    auto target = post.find(key);
    if (target != post.end()) {
        return target->second;
    }
    return "";
}

bool HttpRequest::parse_requestline(const std::string &line) {
    smatch sub_match;
    if (regex_match(line, sub_match, re_requestline)) {
        method = sub_match[1];
        request_uri = sub_match[2];
        version = sub_match[3];
        state = HEADERS;
        LOG_DEBUG("%s", line.c_str());
        return true;
    }
    LOG_ERROR("Invalid RequestLine: \"%s\"", line.c_str());
    return false;
}

void HttpRequest::parse_header(const std::string &line) {
    if (line.size() == 0) {
        // 如果是空行，则状态转移到解析HTTP消息体(body)
        state = BODY;
        return;
    }
    smatch sub_match;
    if (regex_match(line, sub_match, re_header)) {
        std::string field_name = str_lower(sub_match[1]);
        headers[field_name] = sub_match[2];
    }
}

void HttpRequest::parse_body(const std::string &content) {
    body = content;
    if (method == "POST") {
        parse_post();
    }
    state = FINISH;
    LOG_DEBUG("body length: %d", content.size());
}

void HttpRequest::parse_post() {
    if (method != "POST") return;
    if (headers["Content-Type"] == "application/x-www-form-urlencoded") {
        parse_form_urlencoded();
    }
}

void HttpRequest::parse_form_urlencoded() {
    if (body.size() == 0) return;
    std::string tmp, key;
    int byte;
    for (int i=0; i<body.size(); ++i) {
        char ch = body[i];
        switch (ch) {
            case '+': {
                tmp.push_back(' ');
                break;
            }
            case '=': {
                key = tmp;
                tmp.clear();
                break;
            }
            case '&': {
                post[key] = tmp;
                key.clear();
                tmp.clear();
                break;
            }
            case '%': {
                byte = convert_hex(body[i+1])*16 + convert_hex(body[i+2]);
                tmp.push_back(byte);
                i += 2;
                break;
            }
            default: {
                tmp.push_back(ch);
            }
        }
    }
    if (tmp.size() > 0) {
        post[key] = tmp;
    }
}

void HttpRequest::parse_uri() {
    /**
     * Request-URI    = "*" | absoluteURI | abs_path | authority
    */
    if (request_uri[0] == '/') {
        // abs_path
        auto pos = request_uri.find_first_of('?');
        auto tmp = request_uri.substr(0, pos);
        for (int i=0; i<tmp.size(); ++i) {
            if (tmp[i] == '%') {
                auto byte = convert_hex(tmp[i+1])*16 + convert_hex(tmp[i+2]);
                path.push_back(byte);
                i += 2;
            } else {
                path.push_back(tmp[i]);
            }
        }
        if (path == "/") {
            path = "/index.html";
        }
    }
}