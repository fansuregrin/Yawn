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

/* 默认的 HTML 页面 */
std::unordered_set<std::string> HttpRequest::DEFAULT_HTML {
    "/index", "/register", "/login", "/welcome", "/video", "/picture"
};

/* 默认的 HTML 标签 */
std::unordered_map<std::string,int> HttpRequest::DEFAULT_HTML_TAG {
    {"/register.html", 0}, {"/login.html", 1}
};

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

void HttpRequest::str_lower(std::string &str) {
    for (auto ch:str) {
        if (ch >= 'A' && ch <= 'Z') {
            ch = ch - 32;
        }
    }
}

std::string HttpRequest::str_lower(const std::string &str) {
    std::string new_str = str;
    for (auto ch:new_str) {
        if (ch >= 'A' && ch <= 'Z') {
            ch = ch - 32;
        }
    }
    return new_str;
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
            parse_body(std::string(buf.peek(), buf.begin_write_const()));
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
                parse_path();
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
        path = sub_match[2];
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
        std::string field_name = sub_match[1];
        str_lower(field_name);
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
        if (DEFAULT_HTML_TAG.find(path) != DEFAULT_HTML_TAG.end()) {
            int tag = DEFAULT_HTML_TAG[path];
            LOG_DEBUG("Hit HTML tag: %d, the path is: %s", tag, path.c_str());
            // login or register
            if (tag == 0 || tag == 1) {
                if (post.count("username")==0 || post.count("password")==0) {
                    path = "/error.html"; 
                }
                if (verify_user(post["username"], post["password"], (tag == 1))) {
                    if (tag == 1) {
                        LOG_DEBUG("User [%s] successfully logged in!", post["username"].c_str());
                    } else {
                        LOG_DEBUG("User [%s] successfully registered!", post["username"].c_str());
                    }
                    path = "/welcome.html";
                } else {
                    path = "/error.html";
                }
            }
        }
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

void HttpRequest::parse_path() {
    if (path == "/") {
        path = "/index.html";
    } else if (DEFAULT_HTML.find(path) != DEFAULT_HTML.end()) {
        path += ".html";
    }
}

bool
HttpRequest::verify_user(const std::string &username, const std::string &passwd,
bool is_login) {
    if (username == "" || passwd == "") {
        return false;
    }
    
    LOG_DEBUG("Verifying a user: [username: %s, password: %s]", username.c_str(),
        passwd.c_str());
    MYSQL * conn = nullptr;
    SQLConnRAII(conn, SQLConnPool::get_instance());
    assert(conn);

    std::string safe_username;
    for (auto ch:username) {
        if (ch == '\'') {
            safe_username.push_back('\'');
        }
        safe_username.push_back(ch);
    }
    char sql_stmt[512];
    memset(sql_stmt, '\0', sizeof(sql_stmt));
    
    snprintf(sql_stmt, sizeof(sql_stmt)-1, "SELECT username,password FROM users "
        "WHERE `username`='%s';", username.c_str());
    LOG_DEBUG("%s", sql_stmt);
    if (mysql_query(conn, sql_stmt)) {
        LOG_DEBUG("%s", mysql_error(conn));
        return false;
    }
    MYSQL_RES *res = nullptr;
    res = mysql_store_result(conn);
    if (!res) {
        mysql_free_result(res);
        if (mysql_errno(conn)) {
            LOG_DEBUG("%s", mysql_error(conn));
        }
        return false;
    }
    bool verified = false;
    if (MYSQL_ROW row = mysql_fetch_row(res)) {
        // 查询到指定的用户
        LOG_DEBUG("Get the result: username=%s, passowrd=%s", row[0], row[1]);
        std::string real_passwd(row[1]);
        if (is_login) {
            // 验证登录的用户密码是否正确
            if (passwd == real_passwd) {
                verified = true;
                LOG_DEBUG("User verified.");
            } else {
                LOG_DEBUG("Password error!");
            }
        } else {
            // 注册的用户名已经存在
            LOG_DEBUG("Username already exists!");
        }
        mysql_free_result(res);
        return verified;
    }
    // 没有查询到指定用户
    // 如果是登录用户，则用户名错误，验证失败
    if (is_login) return false;
    // 如果是注册用户，则添加新用户
    /**
     * @todo check password
    */
    snprintf(sql_stmt, sizeof(sql_stmt)-1, "INSERT INTO users(username,password)"
        " VALUES('%s','%s');", username.c_str(), passwd.c_str());
    if (mysql_query(conn, sql_stmt)) {
        LOG_DEBUG("%s", mysql_error(conn));
        return false;
    }
    LOG_DEBUG("User verified.");
    return true;
}