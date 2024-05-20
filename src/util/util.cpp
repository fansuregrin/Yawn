/**
 * @file util.cpp
 * @author Fansure Grin
 * @date 2024-05-20
 * @brief source file for utilities
*/
#include "util.h"

std::string http_gmt() {
    auto now = std::time(nullptr);
    auto now_tm = std::gmtime(&now);
    char gmt[40] = {0};
    // Date: <day-name>, <day> <month> <year> <hour>:<minute>:<second> GMT
    char fmt[] = "%a, %d %b %Y %H:%M:%S GMT";
    std::strftime(gmt, sizeof(gmt)-1, fmt, now_tm);
    return gmt;
}

std::string http_gmt(time_t tm_) {
    auto now_tm = std::gmtime(&tm_);
    char gmt[40] = {0};
    // Date: <day-name>, <day> <month> <year> <hour>:<minute>:<second> GMT
    char fmt[] = "%a, %d %b %Y %H:%M:%S GMT";
    std::strftime(gmt, sizeof(gmt)-1, fmt, now_tm);
    return gmt;
}

int hexch2dec(char ch) {
    if (ch <= 'F' && ch >= 'A') return ch - 'A' + 10;
    if (ch <= 'f' && ch >= 'a') return ch - 'a' + 10;
    if (ch <= '9' && ch >= '0') return ch - '0';
    return -1;
}

char dec2hexch(int num) {
    if (num >= 0 && num <= 9) {
        return '0' + num;
    }
    if (num >= 10 && num <= 15) {
        return 'a' + (num - 10);
    }
    return -1;
}

std::string str_lower(std::string str) {
    for (auto &ch:str) {
        if (ch >= 'A' && ch <= 'Z') {
            ch += 32;
        }
    }
    return str;
}