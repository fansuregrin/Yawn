/**
 * @file config.h
 * @author Fansure Grin
 * @date 2024-03-31
 * @brief header file for config
*/
#ifndef CONFIG_H
#define CONFIG_H

#include <unordered_map>
#include <string>

using std::unordered_map;
using std::string;

class Config {
public:
    friend std::ostream& operator<<(std::ostream &os, const Config &cfg);
public:
    using size_type = unordered_map<string,string>::size_type;

    Config(const string &filename);
    ~Config() = default;

    void update(const string &key, const string &val);
    void add(const string &key, const string &val);
    string get_string(const string &key, const string &default_val="") const;
    int get_integer(const string &key, int default_val=0) const;
    double get_float(const string &key, double default_val=0.0) const;
    bool get_bool(const string &key, bool default_val=false) const;
    size_type items_num() const;
private:
    bool parse_file(const string &filename);
    bool parse_line(const string &line);
    void gen_default();
    bool is_valid(const string &kv);

    unordered_map<string,string> tb; // 存储键值对的表
};


#endif // CONFIG_H