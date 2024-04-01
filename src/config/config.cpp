/**
 * @file config.cpp
 * @author Fansure Grin
 * @date 2024-03-31
 * @brief source file for config
*/
#include <iostream>
#include <fstream>
#include "config.h"
#include "../log/log.h"


std::ostream& operator<<(std::ostream &os, const Config &cfg) {
    for (auto item:cfg.tb) {
        os << item.first << ": " << item.second << '\n';
    }
    os << std::flush;
    return os;
}

Config::Config(const string &filename) {
    if (parse_file(filename)) {
        LOG_INFO("Load config from \"%s\".", filename.c_str());
    } else {
        LOG_WARN("Failed to load config from \"%s\"! "
            "Use default config instead!", filename.c_str());
        gen_default();
    }
}

void Config::update(const string &key, const string &val) {
    if (is_valid(key) && is_valid(val)) {
        auto target = tb.find(key);
        if (target != tb.end()) {
            target->second = val;
        }
    }
}

void Config::add(const string &key, const string &val) {
    if (is_valid(key) && is_valid(val)) {
        tb.insert(std::make_pair(key, val));
    }
}

string Config::get_string(const string &key, const string &default_val) const {
    auto target = tb.find(key);
    if (target == tb.end()) {
        return default_val;
    }
    return target->second;
}

int Config::get_integer(const string &key, int default_val) const {
    auto target = tb.find(key);
    if (target == tb.end()) {
        return default_val;
    }
    int ret = default_val;
    try {
        ret = std::stoi(target->second);
    } catch (const std::exception &ex) {
        LOG_DEBUG("%s", ex.what());
    }
    return ret;
}

double Config::get_float(const string &key, double default_val) const {
    auto target = tb.find(key);
    if (target == tb.end()) {
        return default_val;
    }
    double ret = default_val;
    try {
        ret = std::stod(target->second);
    } catch (const std::exception &ex) {
        LOG_DEBUG("%s", ex.what());
    }
    return ret;
}

bool Config::get_bool(const string &key, bool default_val) const {
    auto target = tb.find(key);
    if (target == tb.end()) {
        return default_val;
    }
    if (target->second == "true") {
        return true;
    } else if (target->second == "false") {
        return false;
    } else {
        return default_val;
    }
}

Config::size_type  Config::items_num() const {
    return tb.size();
}

bool Config::parse_file(const string &filename) {
    std::ifstream fs(filename);
    if (!fs.is_open()) {
        LOG_DEBUG("Failed to open config file: \"%s\"!", filename.c_str());
        return false;
    }

    string line;
    while (std::getline(fs, line)) {
        parse_line(line);
    }
    bool status = true;
    if (!fs.eof()) {
        status = false;
    }
    fs.close();
    return status;
}

bool Config::parse_line(const string &line) {
    auto equal_sign_pos = line.find_first_of('=');
    if (equal_sign_pos == string::npos) {
        // 缺失 "="，不合法的一行配置，丢弃之
        return false;
    }
    auto commet_start = line.find_first_of('#');
    if (commet_start != string::npos && commet_start < equal_sign_pos) {
        // 注释起始符 "#" 位于 键值分隔符 "=" 之前，也是非法的一行配置
        return false;
    }
    auto end = line.size();
    if (commet_start != string::npos) {
        end = commet_start;
    }

    string::size_type i = 0, j = 0;
    while (i<equal_sign_pos && std::isspace(line[i])) {
        ++i;
    }
    if (i == equal_sign_pos) {
        // 键(key)为空，不合法
        return false;
    }
    for (j=equal_sign_pos-1; j>i; --j) {
        if (!std::isspace(line[j])) {
            break;
        }
    }
    string key = line.substr(i, j-i+1);
    
    i = equal_sign_pos + 1;
    while (i<end && std::isspace(line[i])) {
        ++i;
    }
    if (i == end) {
        // 值(value)为空，不合法
        return false;
    }
    for (j=end-1; j>i; --j) {
        if (!std::isspace(line[j])) {
            break;
        }
    }
    string val = line.substr(i, j-i+1);

    tb[key] = val;
    return true;
}

void Config::gen_default() {
    tb.insert(
        {
            {"listen_ip", "0.0.0.0"},
            {"listen_port", "6789"},
            {"timeout", "60000"},
            {"open_linger", "true"},
            {"trig_mode", "3"},
            {"sql_host", "localhost"},
            {"sql_port", "3306"},
            {"conn_pool_num", "10"},
            {"open_log", "true"},
            {"log_level", "1"},
            {"log_queue_size", "1000"},
            {"log_path", "/tmp/webserver_logs"},
            {"src_dir", "/var/www/html"},
            {"thread_pool_num", "8"}
        }
    );
}

bool Config::is_valid(const string &kv) {
    for (auto ch:kv) {
        if (!std::isspace(ch)) {
            return true;
        }
    }
    return false;
}