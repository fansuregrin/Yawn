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
    {505, "HTTP Version not supported"}
};

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

void HttpResponse::init(const string &src_dir_, string &path_, bool is_keep_alive_,
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

void HttpResponse::error_content(Buffer &buf, const string &msg) {
    
}

void HttpResponse::make_response(Buffer &buf) {
    check_request_src();
    add_statusline(buf);
    add_headers(buf);
}

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
    headers << "Content-Type: " << get_file_type() << "\r\n";
    buf.append(headers.str());
    add_content_length(buf);
    buf.append("\r\n", 2);
}

void HttpResponse::check_request_src() {
    string real_path = src_dir + path;
    int ret = stat(real_path.c_str(), &mm_file_stat);
    if (ret == -1) {
        if (errno == ENOENT) {
            status_code = 404;
        } else {
            status_code = 500;
        }
    } else if (S_ISDIR(mm_file_stat.st_mode)) {
        // 请求的是一个目录，请求目标不存在
        status_code = 404;
    } else if (!(mm_file_stat.st_mode & S_IROTH)) {
        status_code = 403;
    } else if (status_code == -1) {
        status_code = 200;
    }
}

string HttpResponse::get_file_type() {
    const auto pos = path.find_last_of('.');
    if (pos == string::npos) {
        return "text/plain";
    }
    string suffix = path.substr(pos);
    if (SUFFIX_TYPE.count(suffix)) {
        return SUFFIX_TYPE.find(suffix)->second;
    }
    return "text/plain";
}

void HttpResponse::add_content_length(Buffer &buf) {
    string real_path = src_dir + path;
    int fd = open(real_path.c_str(), O_RDONLY);
    if (fd < 0) {
        return;
    }
    int *ret = (int *)mmap(nullptr, mm_file_stat.st_size, PROT_READ,
        MAP_PRIVATE, fd, 0);
    if (*ret == -1) {
        close(fd);
        return;
    }
    mm_file = (char *)ret;
    close(fd);
    buf.append("Content-Length: " + std::to_string(mm_file_stat.st_size) + "\r\n");
}