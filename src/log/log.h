#ifndef LOG_H
#define LOG_H

#include <string>
#include <thread>
#include <mutex>
#include <queue>
#include <memory>
#include <vector>
#include <unistd.h>
#include "../blocking_queue/blocking_queue.hpp"
#include "../util/util.h"


#define LOG_DEBUG(fmt, ...) do {\
    Log(LogEvent(LogLevel::DEBUG, __FILE__, __LINE__, getpid(), gettid()),\
    fmt, ##__VA_ARGS__); } while(0)
#define LOG_INFO(fmt, ...)  do {\
    Log(LogEvent(LogLevel::INFO,  __FILE__, __LINE__, getpid(), gettid()),\
    fmt, ##__VA_ARGS__); } while(0)
#define LOG_WARN(fmt, ...)  do {\
    Log(LogEvent(LogLevel::WARN,  __FILE__, __LINE__, getpid(), gettid()),\
    fmt, ##__VA_ARGS__); } while(0)
#define LOG_ERROR(fmt, ...) do {\
    Log(LogEvent(LogLevel::ERROR, __FILE__, __LINE__, getpid(), gettid()),\
    fmt, ##__VA_ARGS__); } while(0)


enum LogLevel {
    UNKNOWN = 0,
    DEBUG,
    INFO,
    WARN,
    ERROR,
    LOG_LEVEL_COUNT
};

std::string LogLevelToString(LogLevel lv);

LogLevel StringToLogLevel(const std::string &lv);

class LogEvent {
public:
    using pid_t = decltype(getpid());
    using tid_t = decltype(gettid());

    LogEvent(LogLevel log_lv, const std::string &filename, int line_no, pid_t pid, tid_t tid)
    : m_lv(log_lv), m_filename(filename), m_line_no(line_no), m_pid(pid), m_tid(tid) {}

    LogLevel GetLogLevel() { return m_lv; }

    std::string GetFilename() { return m_filename; }

    int GetLineNumber() { return m_line_no; }    

    std::string ToString();

private:
    LogLevel m_lv;           // 日志级别
    std::string m_filename;  // 文件名
    int m_line_no;           // 行号
    pid_t m_pid;             // 进程 id
    tid_t m_tid;             // 线程 id
};

class AsyncLogger {
public:
    static constexpr uint8_t LOG_TYPE_STDOUT = 1;   // 
    static constexpr uint8_t LOG_TYPE_FILE = 2;
    static constexpr uint8_t LOG_TYPE_STDOUT_FILE = 3;

    static AsyncLogger &GetInstance();

    void Init(uint8_t type, const std::string &logdir, const std::string &filename,
        int max_file_size, LogLevel log_level, std::size_t queue_size);

    void PushLog(const std::string &log_str);

    void CloseLogger();

    bool Closed() const;

    LogLevel GetLogLevel();

private:
    static constexpr uint8_t STDOUT_MASK = 1;
    static constexpr uint8_t FILE_MASK = 2;
    static const char * const ext;

    AsyncLogger();

    virtual ~AsyncLogger();

    void AsyncWrite();

    // logger 的输出类型：
    //   - LOG_TYPE_STDOUT：只输出到标准输出 (STDOUT)
    //   - LOG_TYPE_FILE：只输出到日志文件
    //   - LOG_TYPE_STDOUT_FILE：输出到 STDOUT 和文件
    uint8_t m_type;
    LogLevel m_log_level;    // 日志级别
    std::string m_filename;  // 日志文件名称
    int m_seq_no;            // 日志文件序号
    std::string m_logdir;    // 日志目录
    int m_max_file_size;     // 单个日志文件最大容量，单位为字节
    bool m_closed;           // 关闭标志
    bool m_inited;           // 初始化标志
    FILE *m_fp;              // 文件指针
    mutable std::mutex m_mtx;
    std::unique_ptr<std::thread> m_write_thread; // 写日志线程
    std::unique_ptr<BlockingQueue<std::string>> m_queue;
};

template <typename... Args>
void Log(LogEvent &&log_event, const char* fmt, Args&&... args) {
    std::string log_str;
    int msg_len = snprintf(nullptr, 0, fmt, args...);
    if (msg_len > 0) {
        log_str.resize(msg_len + 1);
        snprintf((char *)&log_str[0], msg_len + 1, fmt, args...);
        log_str.pop_back();
    }

    log_str = log_event.ToString() + log_str + "\n";
    AsyncLogger &logger = AsyncLogger::GetInstance();
    if (logger.Closed() || logger.GetLogLevel() > log_event.GetLogLevel()) return;
    logger.PushLog(log_str);
}

#endif // end of LOG_H