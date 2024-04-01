/**
 * @file log.cpp
 * @author Fansure Grin
 * @date 2024-03-25
 * @brief source file for log
*/
#include <cstdio>
#include <ctime>
#include <cstring>
#include <cassert>
#include <cstdarg>
#include <sys/stat.h>
#include <sys/time.h>
#include <string>
#include "log.h"


Log* Log::get_instance() {
    static Log log;
    return &log;
}

void Log::flush_log_thread() {
    get_instance()->async_write();
}

Log::Log()
: line_num(0), level(1), fp(nullptr), is_async(false),
  is_open_(false), blk_deq(nullptr), write_thread(nullptr) {}

Log::~Log() {
    if (write_thread && write_thread->joinable()) {
        while (!blk_deq->empty()) {
            blk_deq->flush();
        }
        blk_deq->close();
        write_thread->join();
    }
    if (fp) {
        lock_guard lck(mtx);
        flush();
        fclose(fp);
    }
}

void Log::append_loglevel_title(level_type level_) {
    switch (level_) {
        case 0: {
            buf.append("[DEBUG]: ", 9);
            break;
        }
        case 1: {
            buf.append("[INFO] : ", 9);
            break;
        }
        case 2: {
            buf.append("[WARN] : ", 9);
            break;
        }
        case 3: {
            buf.append("[ERROR]: ", 9);
            break;
        }
        default: {
            buf.append("[INFO] : ", 9);
            break;
        }
    }
}

int Log::get_line_num(FILE * fp_) {
    if (!fp_) return -1;
    char ch;
    int line_num_ = 0;
    while ((ch = fgetc(fp_)) != EOF) {
        if (ch == '\n') ++line_num_;
    }
    return line_num_;
}

void
Log::init(level_type level_, const std::string &path_,
          const std::string &suffix_, int max_queue_size) {
    is_open_ = true;
    level = level_;
    if (max_queue_size > 0) {
        is_async = true;
        if (!blk_deq) {
            std::unique_ptr<BlockingDeque<std::string>> tmp_deq(new 
                BlockingDeque<std::string>(max_queue_size));
            blk_deq = std::move(tmp_deq);
            std::unique_ptr<std::thread> tmp_thread(new 
                std::thread(flush_log_thread));
            write_thread = std::move(tmp_thread);
        }
    } else {
        is_async = false;
    }

    time_t curr_tm = time(nullptr);  // current time
    struct tm curr_tm_human = *localtime(&curr_tm);
    path = path_;
    suffix = suffix_;
    char date[DATE_LEN];
    memset(date, '\0', DATE_LEN);
    snprintf(date, DATE_LEN-1, "%04d_%02d_%02d",
             1900 + curr_tm_human.tm_year, 1 + curr_tm_human.tm_mon,
             curr_tm_human.tm_mday);
    filename = path + '/' + date + suffix;
    today = curr_tm_human.tm_mday;
    
    {
        lock_guard lck(mtx);
        buf.retrieve_all();
        // 关闭已经打开的文件
        if (fp) {
            flush();
            fclose(fp);
        }
        // 打开日志文件
        fp = fopen(filename.c_str(), "a+");
        if (!fp) {
            mkdir(path.c_str(), 0777);
            fp = fopen(filename.c_str(), "a+");
        }
        assert(fp);
        line_num = get_line_num(fp);
    }
}

void Log::write(level_type level_, const char *format, ...) {
    struct timeval now = {0, 0};
    gettimeofday(&now, nullptr);
    struct tm curr_tm = *localtime(&now.tv_sec);
    va_list var_list;

    // 如果日期改变或日志行数已经达到最大,则更换新的日志文件
    while (today != curr_tm.tm_mday || (line_num && line_num%MAX_LINE_NUM==0)) {
        unique_lock lck(mtx);
        lck.unlock();

        char date[DATE_LEN];
        memset(date, '\0', DATE_LEN);
        snprintf(date, DATE_LEN-1, "%04d_%02d_%02d", 1900+curr_tm.tm_year,
                1+curr_tm.tm_mon, curr_tm.tm_mday);
        if (today != curr_tm.tm_mday) {
            filename = path + '/' + date + suffix;
            today = curr_tm.tm_mday;
            line_num = 0;
        } else {
            filename = path + '/' + date + '-' + 
                std::to_string(line_num/MAX_LINE_NUM) + suffix;
        }

        lck.lock();
        flush();
        fclose(fp);
        fp = fopen(filename.c_str(), "a+");
        assert(fp);
        int new_line_num = get_line_num(fp);
        if (new_line_num < MAX_LINE_NUM) {
            line_num += new_line_num;
            break;
        }
        line_num += new_line_num;
    }

    {
        unique_lock lck(mtx);
        ++line_num;
        // 将日期和时间写入缓冲区
        char datetime[64];
        memset(datetime, '\0', sizeof(datetime));
        int len = snprintf(datetime, sizeof(datetime)-1,
            "%04d-%02d-%02d %02d:%02d:%02d.%06ld ",
            1900+curr_tm.tm_year, 1+curr_tm.tm_mon, curr_tm.tm_mday, curr_tm.tm_hour,
            curr_tm.tm_min, curr_tm.tm_sec, now.tv_usec);
        assert(len > 0);
        buf.append(datetime, len);
        append_loglevel_title(level_);
        // 将日志内容写入缓冲区
        va_start(var_list, format);
        len = vsnprintf(buf.begin_write(), buf.writable_bytes(), format, var_list);
        va_end(var_list);
        buf.has_written(len);
        buf.append("\n\0", 2);

        if (is_async && blk_deq && !blk_deq->full()) {
            blk_deq->push_back(buf.retrieve_all_as_str());
        } else {
            fputs(buf.peek(), fp);
            buf.retrieve_all();
        }
    }
}

void Log::flush() {
    if (is_async) {
        blk_deq->flush();
    }
    fflush(fp);
}

Log::level_type Log::get_level() {
    lock_guard lck(mtx);
    return level;
}

void Log::set_level(level_type level_) {
    lock_guard lck(mtx);
    level = level_;
}

bool Log::is_open() {
    return is_open_;
}

void Log::async_write() {
    std::string str;
    while (blk_deq->pop(str)) {
        lock_guard lck(mtx);
        fputs(str.c_str(), fp);
    }
}