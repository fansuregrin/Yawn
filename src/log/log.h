/**
 * @file log.h
 * @author Fansure Grin
 * @date 2024-03-25
 * @brief header file for log
*/
#ifndef LOG_H
#define LOG_H

#include <memory>
#include <string>
#include <thread>
#include <mutex>
#include "../blocking_deque/blocking_deque.hpp"
#include "../buffer/buffer.h"

class Log {
public:
    using lock_guard = std::lock_guard<std::mutex>;
    using unique_lock = std::unique_lock<std::mutex>;
    using level_type = int;

    static Log* get_instance();
    static void flush_log_thread();

    void init(level_type level_, const std::string &path_,
              const std::string &suffix_, int max_queue_size);

    void flush();
    void write(level_type level_, const char *format, ...);

    level_type get_level();
    void set_level(level_type level_);
    bool is_open();
    void async_write();
private:
    Log();
    virtual ~Log();
    void append_loglevel_title(level_type level_);
    int get_line_num(FILE * fp_);

    static const int DATE_LEN = 28;
    static const int MAX_LINE_NUM = 50000;

    std::string path;
    std::string suffix;
    std::string filename;
    int line_num;
    level_type level;
    FILE * fp;
    bool is_async;
    bool is_open_;
    int today;

    Buffer buf;

    std::unique_ptr<BlockingDeque<std::string>> blk_deq;
    std::unique_ptr<std::thread> write_thread;
    std::mutex mtx;
};

#define LOG_BASE(level, format, ...) \
    do {\
        Log *log = Log::get_instance();\
        if (log->is_open() && log->get_level()<=level) {\
            log->write(level, format, ##__VA_ARGS__);\
            log->flush();\
        }\
    } while (0);

#define LOG_DEBUG(format, ...) \
    do {LOG_BASE(0, format, ##__VA_ARGS__)} while(0);
#define LOG_INFO(format, ...) \
    do {LOG_BASE(1, format, ##__VA_ARGS__)} while(0);
#define LOG_WARN(format, ...) \
    do {LOG_BASE(2, format, ##__VA_ARGS__)} while(0);
#define LOG_ERROR(format, ...) \
    do {LOG_BASE(3, format, ##__VA_ARGS__)} while(0);

#endif // LOG_H