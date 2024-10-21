/**
 * @file httpconn.cpp
 * @author Fansure Grin
 * @date 2024-03-30
 * @brief source file for http connection
*/
#include <cstring>
#include <cassert>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include "httpconn.h"
#include "../log/log.h"
#include "../version.h"


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

const std::unordered_map<std::string, std::string> HttpConn::SUFFIX_TYPE = {
    { ".html",  "text/html" },
    { ".xml",   "text/xml" },
    { ".xhtml", "application/xhtml+xml" },
    { ".txt",   "text/plain" },
    { ".rtf",   "application/rtf" },
    { ".pdf",   "application/pdf" },
    { ".doc",   "application/msword" },
    { ".docx",  "application/vnd.openxmlformats-officedocument.wordprocessingml.document"},
    { ".xls",   "application/vnd.ms-excel"},
    { ".xlsx",  "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet"},
    { ".ppt",   "application/vnd.ms-powerpoint"},
    { ".pptx",  "application/vnd.openxmlformats-officedocument.presentationml.presentation"},
    { ".ico",   "image/vnd.microsoft.icon"},
    { ".tif",   "image/tiff"},
    { ".tiff",  "image/tiff"},
    { ".svg",   "image/svg+xml"},
    { ".png",   "image/png" },
    { ".webp",  "image/webp"},
    { ".gif",   "image/gif" },
    { ".jpg",   "image/jpeg" },
    { ".jpeg",  "image/jpeg" },
    { ".mp3",   "audio/mpeg"},
    { ".mpeg",  "video/mpeg"},
    { ".mpv",   "video/mpv" },
    { ".mp4",   "video/mp4" },
    { ".avi",   "video/x-msvideo" },
    { ".gz",    "application/x-gzip" },
    { ".tar",   "application/x-tar" },
    { ".rar",   "application/vnd.rar"},
    { ".7z",    "application/x-7z-compressed"},
    { ".css",   "text/css "},
    { ".js",    "text/javascript "},
    { ".json",  "application/json"},
    { ".woff",  "font/woff"},
    { ".woff2", "font/woff2"},
    { ".ttf",   "font/ttf"},
    { ".otf",   "font/otf"},
    { ".eot",   "application/vnd.ms-fontobject"}
};

const std::unordered_map<int,std::string> HttpConn::STATUS_TEXT {
    {200, "OK"},
    {304, "Not Modified"},
    {400, "Bad Request"},
    {403, "Forbidden"},
    {404, "Not Found"},
    {500, "Internal Server Error"},
    {505, "HTTP Version Not Supported"}
};

/**
 * @todo 改为可配置的
*/
const std::unordered_map<int,std::string> HttpConn::CODE_PATH {
    {400, "/400.html"},
    {403, "/403.html"},
    {404, "/404.html"},
    {500, "/500.html"}
};

HttpConn::HttpConn(): fd(-1), is_close(true), iov_cnt(0), state(PARSE_STATE::REQUEST_LINE) {
    bzero(ip, sizeof(ip));
    bzero(&addr, sizeof(addr));
    std::memset(&mm_file_stat, '\0', sizeof(mm_file_stat));
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
    unmap_file();
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

bool HttpConn::is_keep_alive() const {
    auto target = request.headers.find("connection");
    return (
        target != request.headers.end() && 
        str_lower(target->second) == "keep-alive");
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
        response.status_code = 200;
    } else if (parse_res == PARSE_RESULT::ERROR) {
        response.status_code = 400;
    } else if (parse_res == PARSE_RESULT::NOT_FINISH || parse_res == PARSE_RESULT::EMPTY) {
        return false;
    }

    // 响应的状态行、头部和响应体
    make_response();
    iov[0].iov_base = const_cast<char*>(write_buf.peek());
    iov[0].iov_len = write_buf.readable_bytes();
    iov_cnt = 1;
    // 响应要发送的文件
    if (get_mm_file_len() > 0 && mm_file) {
        iov[1].iov_base = mm_file;
        iov[1].iov_len = get_mm_file_len();
        iov_cnt = 2;
    }
    LOG_INFO(
        // request-line response-code content-length
        "\"%s %s HTTP/%s\" %d %ld",
        request.get_method().c_str(), request.get_path().c_str(),
        request.get_version().c_str(), response.status_code,
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

void HttpConn::make_response() {
    response.init();
    if (!request.path.empty()) {
        // 用户请求的资源路径非空，则检查资源文件并尝试将其映射到内存
        // 检查资源文件和映射过程都可能会出错，出错会设置相应的状态码
        if (!check_resource_and_map(src_dir + request.path)) {
            set_err_content();
        }
    }
    set_status_line();
    set_headers();
    write_buf.append(response.get_status_line());
    write_buf.append(response.get_headers());
    if (!response.body.empty()) {
        write_buf.append(response.body);
    }
}

bool HttpConn::check_resource_and_map(const std::string &fp) {
    int ret = stat(fp.c_str(), &mm_file_stat);
    if (ret == -1) {
        if (errno == ENOENT) {  
            // 请求的资源不存在，设置 Not Found 错误码
            response.status_code = 404;
        } else {
            // 其他错误，设置 Internal Server Error 错误码
            response.status_code = 500;
        }
        return false;
    } else if (S_ISDIR(mm_file_stat.st_mode)) {
        // 请求的资源是一个目录，设置 Not Found 错误码
        response.status_code = 404;
        return false;
    } else if (!(mm_file_stat.st_mode & S_IROTH)) {
        // 请求的资源没有读取权限，设置 Forbidden 错误码
        response.status_code = 403;
        // 如果不想让客户端知道这个资源是没有权限访问的，可以返回404
        // 从而让客户端认为要访问的资源不存在，而不是禁止访问
        // "An origin server that wishes to "hide" the current existence of a
        // forbidden target resource MAY instead respond with a status code of
        // 404 (Not Found)."  -- from RFC7231 6.5.3
        // status_code = 404;
        return false;
    }

    // 处理客户端的条件请求
    auto req_etag = request.get_header("if-none-match");
    auto tv_ = mm_file_stat.st_mtim.tv_sec;
    auto etag = dec2hexstr(tv_) + '-' + dec2hexstr(mm_file_stat.st_size);
    if (req_etag == etag) {
        response.status_code = 304;
        return true;
    }

    // 请求的资源没有问题
    // 将客户端请求的资源文件映射到内存
    if (!map_file(fp)) {
        // 映射失败，设置 Internal Server Error 错误码
        response.status_code = 500;
        return false;
    }

    response.headers["content-type"] = get_file_type(fp);
    response.headers["content-length"] = std::to_string(mm_file_stat.st_size);
    return true;
}

bool HttpConn::map_file(const std::string &fp) {
    int fd = open(fp.c_str(), O_RDONLY);
    if (fd < 0) {
        // 打开文件失败
        return false;
    }
    void *ret = mmap(nullptr, mm_file_stat.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (ret == (void *)-1) {
        // 映射失败
        close(fd);
        return false;
    }
    mm_file = (char *)ret;
    close(fd);
    return true;
}

std::string HttpConn::get_file_type(const std::string &fp) {
    const auto pos = fp.find_last_of('.');
    if (pos == std::string::npos) {
        return "text/html";
    }
    std::string suffix = fp.substr(pos);
    const auto it = SUFFIX_TYPE.find(suffix);
    if (it != SUFFIX_TYPE.end()) {
        return it->second;
    }
    return "text/html";
}

void HttpConn::set_err_content() {
    if (response.status_code == 200) return;
    const auto &target = CODE_PATH.find(response.status_code);
    if (target != CODE_PATH.end()) {
        if (!check_resource_and_map(src_dir + target->second)) {
            response.body = get_default_err_content();
            response.headers["content-type"] = "text/html";
            response.headers["content-length"] = std::to_string(response.body.size());
        }
    } else {
        response.body = get_default_err_content();
        response.headers["content-type"] = "text/html";
        response.headers["content-length"] = std::to_string(response.body.size());
    }
}

std::string HttpConn::get_default_err_content() {
    std::ostringstream content;
    auto status_code = response.status_code;
    content << "<html>\n<head><title>" 
            << status_code << ' ' << STATUS_TEXT.at(status_code)
            << "</title></head>\n<body>\n<center><h1>"
            << status_code << ' ' << STATUS_TEXT.at(status_code)
            << "</h1></center>\n<hr><center>"
            << _VENDOR_NAME << "/" << _VERSION_STRING
            << "</center>\n</body>\n</html>";
    return content.str();
}

void HttpConn::set_status_line() {
    response.version = "HTTP/1.1";
    auto &status_code = response.status_code;
    const auto &it = STATUS_TEXT.find(status_code);
    if (it != STATUS_TEXT.end()) {
        response.status_text = it->second;
    } else {
        status_code = 400;
        response.status_text = "Bad Request";
    }
}

void HttpConn::set_headers() {
    auto &headers = response.headers;
    headers["connection"] = is_keep_alive() ? "keep-alive" : "close";
    if (mm_file || response.status_code == 304) {
        auto tv_ = mm_file_stat.st_mtim.tv_sec;
        headers["last-modified"] = http_gmt(tv_);
        headers["etag"] = dec2hexstr(tv_) + '-' + dec2hexstr(mm_file_stat.st_size);
    }
    headers["date"] = http_gmt();
    headers["server"] = std::string(_VENDOR_NAME) + '/' + _VERSION_STRING;
}

void HttpConn::unmap_file() {
    if (mm_file) {
        munmap(mm_file, mm_file_stat.st_size);
        memset(&mm_file_stat, '\0', sizeof(mm_file_stat));
        mm_file = nullptr;
    }
}

const char * HttpConn::get_mm_file() const {
    return mm_file;
}

decltype(stat::st_size) HttpConn::get_mm_file_len() const {
    return mm_file_stat.st_size;
}