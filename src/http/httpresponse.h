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

    void init(const string &src_dir_, const string &path_, bool is_keep_alive_ = false,
        int status_code_ = -1);
    void unmap_file();
    void make_response(Buffer &buf);
    char* get_file();
    size_t filelen() const;

    int get_status_code() const { return status_code; }
    int get_content_length() const { return content_length; }
private:
    void add_statusline(Buffer &buf);
    void add_headers(Buffer &buf);
    void set_err_content();
    bool check_resource_and_map(const std::string &fp);
    bool map_file(const std::string &fp);
    string get_file_type(const std::string &fp);
    string get_default_err_content();

    int status_code;
    bool is_keep_alive;
    string path;
    string src_dir;
    string body;
    string content_type;
    size_t content_length{0};
    char * mm_file;  // 文件映射到内存中的地址
    struct stat mm_file_stat;

    static const unordered_map<string,string> SUFFIX_TYPE;
    static const unordered_map<int,string> STATUS_TEXT;
    static const unordered_map<int,string> CODE_PATH;
};

#endif // HTTP_RESPONSE_H