/**
 * @file util.h
 * @author Fansure Grin
 * @date 2024-05-20
 * @brief header file for utilities
*/
#ifndef UTIL_H
#define UTIL_H

#include <ctime>
#include <string>

/**
 * @brief 为http头部的Date字段生成当前时间的格式化字符串
 * @details 
 * Date: <day-name>, <day> <month> <year> <hour>:<minute>:<second> GMT
 * 
 * @return 格式化的时间字符串
*/
std::string http_gmt();

/**
 * @brief 为http头部的Date字段生成指定时间的格式化字符串
 * @param tm_ 指定的时间
 * @details 
 * Date: <day-name>, <day> <month> <year> <hour>:<minute>:<second> GMT
 * 
 * @return 格式化的时间字符串
*/
std::string http_gmt(time_t tm_);

/**
 * @brief 将一个十六进制字符转换成十进制数
 * @param ch 十六进制字符 (ABCDEFabcdef)
 * @return 一个十进制数
*/
int hexch2dec(char ch);

/**
 * @brief 将一个十进制数字转换成十六进制字符
 * @param ch 十进制数字 (0~15)
 * @return 一个十六进制字符
*/
char dec2hexch(int num);

/**
 * @brief 将字符串中的大写拉丁字母转化为小写拉丁字母
 * @param str 一个字符串
 * @return 只包含小写字母的字符串
*/
std::string str_lower(std::string str);

/**
 * @brief 将十进制数字转换成十六进制字符表示的字符串
 * @param num 十进制数字
 * @return 十六进制字符表示的字符串
*/
template <typename T>
std::string dec2hexstr(T num) {
    constexpr auto cnt = sizeof(T) * 8 / 4;
    char hexstr[cnt+1] = {0};
    for (int i=0; i<cnt; ++i) {
        hexstr[cnt-i-1] = dec2hexch(num & 0xf);
        num = (num >> 4);
    }
    // 去除前置的零
    int curr = 0;
    while (curr<cnt-1 && hexstr[curr]=='0') {
        ++curr;
    }
    return hexstr+curr;
}

#endif