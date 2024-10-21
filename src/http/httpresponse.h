/**
 * @file httprequest.h
 * @author Fansure Grin
 * @date 2024-03-29
 * @brief header file for http-response
*/
#ifndef HTTP_RESPONSE_H
#define HTTP_RESPONSE_H

#include <unordered_map>
#include <string>

class HttpResponse {
public:
    friend class HttpConn;

    HttpResponse();
    ~HttpResponse();

    void init();
    int get_status_code() const;
    size_t get_content_length() const;
    std::string get_status_line() const;
    std::string get_headers() const;

private:
    int status_code;
    std::string status_text;
    std::string version;
    std::unordered_map<std::string,std::string> headers;
    std::string body;
};

#endif // HTTP_RESPONSE_H