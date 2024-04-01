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
#include <sys/stat.h>
#include "../buffer/buffer.h"

using std::unordered_map;
using std::string;

class HttpResponse {
public:
    HttpResponse();
    ~HttpResponse();

    void init(const string &src_dir_, string &path_, bool is_keep_alive_ = false,
        int status_code_ = -1);
    void unmap_file();
    void make_response(Buffer &buf);
    char* get_file();
    size_t filelen() const;
    void error_content(Buffer &buf, const string &msg);

    int get_status_code() const { return status_code; }
private:
    void add_statusline(Buffer &buf);
    void add_headers(Buffer &buf);
    void check_request_src();
    void add_content_length(Buffer &buf);
    string get_file_type();

    int status_code;
    bool is_keep_alive;
    string path;
    string src_dir;
    char * mm_file;
    struct stat mm_file_stat;

    static const unordered_map<string,string> SUFFIX_TYPE;
    static const unordered_map<int,string> STATUS_TEXT;
    static const unordered_map<int,string> CODE_PATH;
};

#endif // HTTP_RESPONSE_H