/**
 * @file httprequest.cpp
 * @author Fansure Grin
 * @date 2024-03-29
 * @brief source file for http-response
*/
#include "httpresponse.h"


HttpResponse::HttpResponse() : status_code(200), version("HTTP/1.1") {}

HttpResponse::~HttpResponse() {}

void HttpResponse::init() {
    status_code = 200;
    version.clear();
    status_text.clear();
    headers.clear();
    body.clear();
}

int HttpResponse::get_status_code() const {
    return status_code;
}

size_t HttpResponse::get_content_length() const {
    const auto &it = headers.find("content-length");
    size_t content_length = 0;
    if (it != headers.end() && !it->second.empty()) {
        content_length = std::stol(it->second);
    }
    return content_length;
}

std::string HttpResponse::get_status_line() const {
    // Status-Line = HTTP-Version SP Status-Code SP Reason-Phrase CRLF
    return version + ' ' + std::to_string(status_code) + ' ' + status_text + "\r\n";
}

std::string HttpResponse::get_headers() const {
    std::string headers_str;
    for (const auto &p : headers) {
        headers_str.append(p.first + ": " + p.second + "\r\n");
    }
    headers_str.append("\r\n");
    return headers_str;
}