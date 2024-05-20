/**
 * @file webserver.h
 * @author Fansure Grin
 * @date 2023-03-30
 * @brief head files for webserver
*/
#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <memory>
#include <unordered_map>
#include "../timer/heap_timer.h"
#include "../pool/threadpool.hpp"
#include "../epoller/epoller.h"
#include "../http/httpconn.h"
#include "../log/log.h"
#include "../pool/sqlconnpool.h"
#include "../config/config.h"


class WebServer {
public:
    using level_type = Log::level_type;

    WebServer(
    // socket
    const string &ip_, int listen_port_, int timeout_, bool open_linger_,
    int trig_mode_,
    // database
    bool enable_db_, const char *sql_host, int sql_port, const char *sql_username,
    const char *sql_passwd, const char *db_name, int conn_pool_num,
    // log
    bool open_log, level_type log_level, int log_queue_size, const char *log_path,
    const string &src_dir_, int thread_pool_num);

    WebServer(const Config &cfg);
    
    ~WebServer();

    void start();
private:
    void init(
        // socket
        const string &ip_, int listen_port_, int timeout_, bool open_linger_,
        int trig_mode_,
        // database
        bool enable_db_, const char *sql_host, int sql_port, const char *sql_username,
        const char *sql_passwd, const char *db_name, int conn_pool_num,
        // log
        bool open_log, level_type log_level, int log_queue_size, const char *log_path,
        
        const string &src_dir_, int thread_pool_num
    );
    
    bool init_socket();
    void init_event_mode(int trig_mode);

    void add_client(int fd, const sockaddr_in &addr);
    void close_conn(HttpConn *client);
    void extend_time(HttpConn *client);
    void send_error_msg(int fd, const char *msg);
    void deal_listen();
    void deal_read(HttpConn *client);
    void on_read(HttpConn *client);
    void deal_write(HttpConn *client);
    void on_write(HttpConn *client);
    void on_process(HttpConn *client);

    static int set_nonblocking(int fd);
    static const int MAX_FD = 65536;

    std::string ip;  // 监听的 IP 地址
    int listen_port; // 监听的端口号
    int listen_fd;
    bool open_linger; // 是否开启 linger
    int timeout;     // 超时时间，单位为毫秒
    bool is_close;
    bool enable_db;  // 是否启用数据库连接池
    std::string src_dir;   // 静态资源的根目录
    uint32_t listen_event;  // 与监听socket相关联的事件
    uint32_t conn_event;    // 与连接socket相关联的事件
    std::unique_ptr<TimeHeap> tm_heap;
    std::unique_ptr<ThreadPool> thread_pool;
    std::unique_ptr<Epoller> epoller;
    std::unordered_map<int,HttpConn> users;
};

#endif // WEBSERVER_H