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


using std::regex;
using std::regex_match;
using std::smatch;

std::string HttpConn::src_dir;
bool HttpConn::is_ET;
std::atomic<int> HttpConn::conn_count;

/**
 * 匹配 HTTP 请求行的正则表达式 
 * Request-Line   = Method SP Request-URI SP HTTP-Version CRLF 
 * HTTP-Version   = "HTTP" "/" 1*DIGIT "." 1*DIGIT
*/
const regex HttpConn::re_requestline("^([^ ]*) ([^ ]*) HTTP/(\\d+\\.\\d+)$");

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
const regex HttpConn::re_header("^([^:]*):[ \t]*(.*)$");

HttpConn::HttpConn(): fd(-1), is_close(true), iov_cnt(0), state(PARSE_STATE::REQUEST_LINE) {
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
    ++conn_count;
    write_buf.retrieve_all();
    read_buf.retrieve_all();
    is_close = false;
    LOG_INFO("<client %d, %s:%d> connected! Connection Count: %d", fd, get_ip(),
        get_port(), conn_count.load());
}

void HttpConn::close_conn() {
    response.unmap_file();
    if (!is_close) {
        is_close = true;
        --conn_count;
        close(fd);
        LOG_INFO("<client %d, %s:%d> quited! Connection Count: %d", fd, get_ip(),
            get_port(), conn_count.load());
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
    } while (is_ET || to_write_bytes());

    return total_len;
}

HttpConn::PARSE_RESULT HttpConn::parse(Buffer &buf) {
    if (buf.readable_bytes() <= 0) return PARSE_RESULT::EMPTY;
    
    // 解析请求行和请求头
    const char CRLF[] = "\r\n";
    while (buf.readable_bytes() && state != BODY) {
        const char *data_begin = buf.peek();
        const char *data_end = data_begin + buf.readable_bytes();
        const char *line_end = std::search(data_begin, data_end, CRLF, CRLF+2);
        if (line_end == data_end) {
            // 没有完整的一行数据，需要继续从socket中读入数据到缓冲区
            return PARSE_RESULT::NOT_FINISH;
        }
        std::string line(buf.peek(), line_end);
        if (state == PARSE_STATE::REQUEST_LINE) {
            if (!parse_requestline(line)) {
                return PARSE_RESULT::ERROR;
            }
        } else if (state == PARSE_STATE::HEADERS) {
            parse_header(line);
        }
        buf.retrieve_until(line_end+2);  // 跳过 "\r\n"
    }
    
    // 解析请求体
    long content_length = 0;
    auto content_length_str = request.get_header("content-length");
    if (!content_length_str.empty()) {
        content_length = std::stol(content_length_str);
    }
    if (content_length == 0) {
        state = FINISH;
        return PARSE_RESULT::OK;
    } else if (content_length <= buf.readable_bytes()) {
        parse_body(buf.retrieve_as_str(content_length));
        return PARSE_RESULT::OK;
    } else {
        return PARSE_RESULT::NOT_FINISH;
    }
}

bool HttpConn::parse_requestline(const std::string &line) {
    smatch sub_match;
    if (regex_match(line, sub_match, re_requestline)) {
        request.method = sub_match[1];
        request.request_uri = sub_match[2];
        request.version = sub_match[3];
        parse_uri(request.request_uri);
        state = PARSE_STATE::HEADERS;
        LOG_DEBUG("request line: %s", line.c_str());
        return true;
    }
    LOG_ERROR("invalid request line: \"%s\"", line.c_str());
    return false;
}

bool HttpConn::parse_uri(const std::string &uri) {
    /**
     * Request-URI    = "*" | absoluteURI | abs_path | authority
    */
    if (uri[0] == '/') {
        // abs_path
        auto pos = uri.find_first_of('?');
        auto tmp = uri.substr(0, pos);
        for (decltype(tmp.size()) i=0; i<tmp.size(); ++i) {
            if (tmp[i] == '%') {
                auto byte = hexch2dec(tmp[i+1])*16 + hexch2dec(tmp[i+2]);
                request.path.push_back(byte);
                i += 2;
            } else {
                request.path.push_back(tmp[i]);
            }
        }
        if (request.path == "/") {
            request.path = "/index.html";
        }
    }
    return true;
}

bool HttpConn::parse_header(const std::string &line) {
    if (line.size() == 0) {
        // 如果是空行，则状态转移到解析HTTP消息体(body)
        state = PARSE_STATE::BODY;
        return true;
    }
    smatch sub_match;
    if (regex_match(line, sub_match, re_header)) {
        std::string field_name = str_lower(sub_match[1]);
        request.headers[field_name] = sub_match[2];
        return true;
    }
    return false;
}

void HttpConn::parse_body(const std::string &content) {
    request.body = content;
    if (request.method == "POST") {
        parse_post();
    }
    state = PARSE_STATE::FINISH;
    LOG_DEBUG("request body length: %d", content.size());
}

void HttpConn::parse_post() {
    if (request.method != "POST") return;
    if (request.get_header("content-length") == "application/x-www-form-urlencoded") {
        parse_form_urlencoded();
    }
}

void HttpConn::parse_form_urlencoded() {
    auto body_len = request.body.size();
    if (body_len == 0) return;
    std::string tmp, key;
    int byte;
    for (decltype(body_len) i=0; i<body_len; ++i) {
        char ch = request.body[i];
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
                request.post[key] = tmp;
                key.clear();
                tmp.clear();
                break;
            }
            case '%': {
                byte = hexch2dec(request.body[i+1])*16 + hexch2dec(request.body[i+2]);
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
        request.post[key] = tmp;
    }
}

bool HttpConn::process() {
    if (state == PARSE_STATE::FINISH) {
        request.init();
        state = PARSE_STATE::REQUEST_LINE;
    }
    if (read_buf.readable_bytes() <= 0) {
        return false;
    }
    
    auto parse_res = parse(read_buf);
    if (parse_res == PARSE_RESULT::OK) {
        response.init(src_dir, request, 200);
    } else if (parse_res == PARSE_RESULT::ERROR) {
        response.init(src_dir, 400);
    } else if (parse_res == PARSE_RESULT::NOT_FINISH || parse_res == PARSE_RESULT::EMPTY) {
        return false;
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
    LOG_INFO(
        // request-line response-code content-length
        "\"%s %s HTTP/%s\" %d %d",
        request.get_method().c_str(), request.get_path().c_str(),
        request.get_version().c_str(), response.get_status_code(),
        response.get_content_length()
    );
    LOG_DEBUG("response bytes: %d, file bytes: %d", to_write_bytes(),
        iov[1].iov_len);
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