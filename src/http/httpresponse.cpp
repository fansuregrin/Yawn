/**
 * @file httprequest.cpp
 * @author Fansure Grin
 * @date 2024-03-29
 * @brief source file for http-response
*/
#include <sstream>
#include <cstring>
#include <cassert>
#include <cerrno>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include "httpresponse.h"
#include "../log/log.h"
#include "../version.h"

const unordered_map<string, string> HttpResponse::SUFFIX_TYPE = {
    { ".html",  "text/html" },
    { ".xml",   "text/xml" },
    { ".xhtml", "application/xhtml+xml" },
    { ".txt",   "text/plain" },
    { ".rtf",   "application/rtf" },
    { ".pdf",   "application/pdf" },
    { ".word",  "application/nsword" },
    { ".png",   "image/png" },
    { ".gif",   "image/gif" },
    { ".jpg",   "image/jpeg" },
    { ".jpeg",  "image/jpeg" },
    { ".au",    "audio/basic" },
    { ".mpeg",  "video/mpeg" },
    { ".mpg",   "video/mpeg" },
    { ".avi",   "video/x-msvideo" },
    { ".gz",    "application/x-gzip" },
    { ".tar",   "application/x-tar" },
    { ".css",   "text/css "},
    { ".js",    "text/javascript "},
};

const unordered_map<int,string> HttpResponse::STATUS_TEXT {
    {200, "OK"},
    {400, "Bad Request"},
    {403, "Forbidden"},
    {404, "Not Found"},
    {500, "Internal Server Error"},
    {505, "HTTP Version Not Supported"}
};

/**
 * @todo 改为可配置的
*/
const unordered_map<int,string> HttpResponse::CODE_PATH {
    {400, "/400.html"},
    {403, "/403.html"},
    {404, "/404.html"},
    {500, "/500.html"}
};

HttpResponse::HttpResponse()
: status_code(-1), is_keep_alive(false), path(), src_dir(), mm_file(nullptr) {
    std::memset(&mm_file_stat, '\0', sizeof(mm_file_stat));
}

HttpResponse::~HttpResponse() {
    unmap_file();
}

void HttpResponse::unmap_file() {
    if (mm_file) {
        munmap(mm_file, mm_file_stat.st_size);
        memset(&mm_file_stat, '\0', sizeof(mm_file_stat));
        mm_file = nullptr;
    }
}

void HttpResponse::init(const string &src_dir_, const string &path_, bool is_keep_alive_,
int status_code_) {
    assert(src_dir_ != "");
    unmap_file();
    src_dir = src_dir_;
    path = path_;
    is_keep_alive = is_keep_alive_;
    status_code = status_code_;
}

char* HttpResponse::get_file() {
    return mm_file;
}

size_t HttpResponse::filelen() const {
    return mm_file_stat.st_size;
}

/**
 * @brief 构造响应：添加状态行、头部、消息体到缓冲区。
 * 
 * 如果向客户端返回的消息体需要从文件中获取，则将文件映射到内存。
 *  
 * @param buf 缓冲区
*/
void HttpResponse::make_response(Buffer &buf) {
    body.clear();  // 做出响应之前先把上次响应的内容清空
    if (!path.empty()) {
        // 用户请求的资源路径非空，则检查资源文件并尝试将其映射到内存
        // 检查资源文件和映射过程都可能会出错，出错会设置相应的状态码
        if (!check_resource_and_map(src_dir + path)) {
            set_err_content();
        }
    }
    add_statusline(buf);
    add_headers(buf);
    if (!body.empty()) {
        buf.append(body);
    }
}
/**
 * @brief 根据状态码设置错误页面内容
*/
void HttpResponse::set_err_content() {
    if (status_code == 200) return;
    const auto &target = CODE_PATH.find(status_code);
    if (target != CODE_PATH.end()) {
        if (!check_resource_and_map(src_dir + target->second)) {
            body = get_default_err_content();
            content_type = "text/html";
            content_length = body.size();
        }
    } else {
        body = get_default_err_content();
        content_type = "text/html";
        content_length = body.size();
    }
}

/**
 * @brief 向缓冲区中添加请求行
 * @param buf 缓冲区
*/
void HttpResponse::add_statusline(Buffer &buf) {
    // Status-Line = HTTP-Version SP Status-Code SP Reason-Phrase CRLF
    std::ostringstream statusline;
    statusline << "HTTP/1.1 ";
    const auto &target = STATUS_TEXT.find(status_code);
    if (target != STATUS_TEXT.end()) {
        statusline << status_code << ' ' << target->second;
    } else {
        status_code = 400;
        statusline << "400 " << STATUS_TEXT.find(400)->second;
    }
    statusline << "\r\n";
    buf.append(statusline.str());
}

void HttpResponse::add_headers(Buffer &buf) {
    std::ostringstream headers;
    headers << "Connection: ";
    if (is_keep_alive) {
        headers << "Keep-Alive\r\n" << "Keep-Alive: timeout=5, max=100\r\n";
    } else {
        headers << "Close\r\n";
    }
    headers << "Content-Type: " << content_type << "\r\n"
            << "Content-Length: " << content_length << "\r\n";
    buf.append(headers.str());
    buf.append("\r\n", 2);
}

/**
 * @brief 检查资源文件并将资源文件映射到内存
*/
bool HttpResponse::check_resource_and_map(const std::string &fp) {
    int ret = stat(fp.c_str(), &mm_file_stat);
    if (ret == -1) {
        if (errno == ENOENT) {  
            // 请求的资源不存在，设置 Not Found 错误码
            status_code = 404;
        } else {
            // 其他错误，设置 Internal Server Error 错误码
            status_code = 500;
        }
        return false;
    } else if (S_ISDIR(mm_file_stat.st_mode)) {
        // 请求的资源是一个目录，设置 Not Found 错误码
        status_code = 404;
        return false;
    } else if (!(mm_file_stat.st_mode & S_IROTH)) {
        // 请求的资源没有读取权限，设置 Forbidden 错误码
        status_code = 403;
        // 如果不想让客户端知道这个资源是没有权限访问的，可以返回404
        // 从而让客户端认为要访问的资源不存在，而不是禁止访问
        // "An origin server that wishes to "hide" the current existence of a
        // forbidden target resource MAY instead respond with a status code of
        // 404 (Not Found)."  -- from RFC7231 6.5.3
        // status_code = 404;
        return false;
    }

    // 请求的资源没有问题
    // 将客户端请求的资源文件映射到内存
    if (!map_file(fp)) {
        // 映射失败，设置 Internal Server Error 错误码
        status_code = 500;
        return false;
    }

    content_type = get_file_type(fp);
    content_length = mm_file_stat.st_size;
    return true;
}

/**
 * @brief 将文件映射到内存
 * @param fp 文件路径
*/
bool HttpResponse::map_file(const std::string &fp) {
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

string HttpResponse::get_file_type(const std::string &fp) {
    const auto pos = fp.find_last_of('.');
    if (pos == string::npos) {
        return "text/html";
    }
    string suffix = fp.substr(pos);
    if (SUFFIX_TYPE.count(suffix)) {
        return SUFFIX_TYPE.find(suffix)->second;
    }
    return "text/html";
}

string HttpResponse::get_default_err_content() {
    std::ostringstream content;
    content << "<html>\n<head><title>" 
            << status_code << ' ' << STATUS_TEXT.at(status_code)
            << "</title></head>\n<body>\n<center><h1>"
            << status_code << ' ' << STATUS_TEXT.at(status_code)
            << "</h1></center>\n<hr><center>"
            << _VENDOR_NAME << "/" << _VERSION_STRING
            << "</center>\n</body>\n</html>";
    return content.str();
}