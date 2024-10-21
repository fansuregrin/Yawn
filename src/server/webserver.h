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
    WebServer(const Config &cfg);
    
    ~WebServer();

    void start();
private:
    void init_db_pool(const char *sql_host, int sql_port,
        const char *sql_username, const char *sql_passwd,
        const char *db_name, int conn_pool_num);
    bool init_socket(const string &ip, int listen_port,
        int timeout, bool open_linger, int trig_mode);
    void init_event_mode(int trig_mode);

    void add_client(int fd, const sockaddr_in &addr);
    void close_conn(std::shared_ptr<HttpConn> client);
    void extend_time(std::shared_ptr<HttpConn> client);
    void send_error_msg(int fd, const char *msg);
    void deal_listen();
    void deal_read(std::shared_ptr<HttpConn> client);
    void on_read(std::shared_ptr<HttpConn> client);
    void deal_write(std::shared_ptr<HttpConn> client);
    void on_write(std::shared_ptr<HttpConn> client);
    void on_process(std::shared_ptr<HttpConn> client);

    static int set_nonblocking(int fd);
    
    int max_num_conn;    // 最大连接数量
    std::string m_ip;    // 监听的 IP 地址
    int m_listen_port;   // 监听的端口号
    int m_listen_fd;     // 标识监听 socket 的文件描述符
    bool m_open_linger;  // 是否开启 linger
    int m_timeout;       // 超时时间，单位为毫秒
    bool m_is_close;     // 服务器是否关闭
    bool m_enable_db;  // 是否启用数据库连接池
    std::string m_src_dir;   // 静态资源的根目录
    uint32_t m_listen_event;  // 与监听socket相关联的事件
    uint32_t m_conn_event;    // 与连接socket相关联的事件
    std::unique_ptr<TimeHeap> m_tm_heap; // 时间堆
    std::unique_ptr<ThreadPool> m_thread_pool; // 线程池，存放工作线程
    std::unique_ptr<Epoller> m_epoller;
    // 已连接socket的文件描述符 -> 连接对象
    std::unordered_map<int,std::shared_ptr<HttpConn>> m_clients;
};

#endif // WEBSERVER_H