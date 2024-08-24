/**
 * @file webserver.cpp
 * @author Fansure Grin
 * @date 2023-03-31
 * @brief source files for webserver
*/
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include "webserver.h"


void WebServer::init_db_pool(
    bool enable_db_, const char *sql_host, int sql_port,
    const char *sql_username, const char *sql_passwd,
    const char *db_name, int conn_pool_num
) {
    enable_db = enable_db_;
    if (enable_db_) {
        SQLConnPool::get_instance()->init(sql_host, sql_port, sql_username,
            sql_passwd, db_name, conn_pool_num);
        LOG_INFO("Number of connections in SQL-Pool: %d", conn_pool_num);
    }
}

WebServer::WebServer(const Config &cfg):
is_close(false), tm_heap(new TimeHeap()), epoller(new Epoller()) {
    LOG_INFO("====== Server initialization ======");

    // 初始化 socket
    if (!init_socket(
        cfg.get_string("listen_ip"),
        cfg.get_integer("listen_port"),
        cfg.get_integer("timeout"),
        cfg.get_bool("open_linger"),
        cfg.get_integer("trig_mode")
    )) {
        is_close = true;
    }

    src_dir = cfg.get_string("src_dir");
    LOG_INFO("Resource directory: %s", src_dir.c_str());
    auto thread_count = cfg.get_integer("thread_pool_num");
    thread_pool.reset(new ThreadPool(thread_count));
    LOG_INFO("Number of threads in Thread-Pool: %d", thread_count);

    HttpConn::conn_count = 0;
    HttpConn::src_dir = src_dir;

    // 初始化数据库连接池
    init_db_pool(
        cfg.get_bool("enable_db"),
        cfg.get_string("sql_host").c_str(),
        cfg.get_integer("sql_port"),
        cfg.get_string("sql_username").c_str(),
        cfg.get_string("sql_passwd").c_str(),
        cfg.get_string("db_name").c_str(),
        cfg.get_integer("conn_pool_num"));

    if (is_close) {
        LOG_ERROR("Server initialization error");
        exit(-1);
    }
}

WebServer::~WebServer() {
    close(listen_fd);
    is_close = true;
    SQLConnPool::get_instance()->close();
}

void WebServer::start() {
    int wait_tm = 1;
    if (!is_close) {
        LOG_INFO("====== Server start ======");
    }
    while (!is_close) {
        if (timeout > 0) {
            // 处理定时事件
            wait_tm = tm_heap->get_next_tick();
        }
        int event_cnt = epoller->wait(wait_tm);
        for (int i=0; i<event_cnt; ++i) {
            int fd = epoller->get_event_fd(i);
            uint32_t events = epoller->get_events(i);
            if (fd == listen_fd) {
                deal_listen();
            } else if (events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
                if (users.count(fd)) {
                    close_conn(&users[fd]);
                }
            } else if (events & EPOLLIN) {
                if (users.count(fd)) {
                    deal_read(&users[fd]);
                }
            } else if (events & EPOLLOUT) {
                if (users.count(fd)) {
                    deal_write(&users[fd]);
                }
            } else {
                LOG_ERROR("Unexpected event!");
            }
        }
    }
}

bool WebServer::init_socket(
    const string &ip_, int listen_port_, int timeout_,
    bool open_linger_, int trig_mode_
) {
    ip = ip_;
    listen_port = listen_port_;
    timeout = timeout_;
    open_linger = open_linger_;
    init_event_mode(trig_mode_);

    int ret;
    
    struct sockaddr_in addr;
    if (listen_port > 65535 || listen_port < 1024) {
        LOG_ERROR("Invalid port number: %d (1024 <= port <= 65535)", listen_port);
        return false;
    }
    addr.sin_family = AF_INET;
    inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);
    addr.sin_port = htons(listen_port);

    listen_fd = socket(PF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) {
        LOG_ERROR("Create socket error!");
        return false;
    }
    struct linger opt_linger = {0, 0};
    if (open_linger) {
        // 开启linger选项：如有数据待发送，则延迟关闭
        opt_linger.l_onoff = 1;
        opt_linger.l_linger = 1;
    }
    ret = setsockopt(listen_fd, SOL_SOCKET, SO_LINGER, &opt_linger,
        sizeof(opt_linger));
    if (ret < 0) {
        close(listen_fd);
        LOG_ERROR("Set linger error!");
        return false;
    }
    int optval = 1;
    // 设置地址重用
    ret = setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &optval,
        sizeof(optval));
    if (ret < 0) {
        close(listen_fd);
        LOG_ERROR("Set reuse-address error!");
        return false;
    }

    ret = bind(listen_fd, (sockaddr *)&addr, sizeof(addr));
    if (ret < 0) {
        close(listen_fd);
        LOG_ERROR("Bind %s:%d error!", ip, listen_port);
        return false;
    }

    ret = listen(listen_fd, 6);
    if (ret < 0) {
        close(listen_fd);
        LOG_ERROR("Listen %s:%d error!", ip, listen_port);
        return false;
    }

    ret = epoller->add_fd(listen_fd, listen_event | EPOLLIN);
    if (ret < 0) {
        close(listen_fd);
        LOG_ERROR("Add listen events error!");
        return false;
    }

    set_nonblocking(listen_fd);
    LOG_INFO("Listen on %s:%d, open-linger: %s", ip.c_str(), listen_port,
            (open_linger ? "true" : "false"));
    LOG_INFO("Listen mode: %s, Open connection mode: %s",
        ((listen_event & EPOLLET) ? "ET" : "LT"),
        ((conn_event & EPOLLET) ? "ET" : "LT"));
    return true;
}

void WebServer::init_event_mode(int trig_mode) {
    listen_event = EPOLLRDHUP;
    conn_event = EPOLLONESHOT | EPOLLRDHUP;
    switch (trig_mode) {
        case 0: break;
        case 1:
            conn_event |= EPOLLET;
            break;
        case 2:
            listen_event |= EPOLLET;
            break;
        case 3:
            conn_event |= EPOLLET;
            listen_event |= EPOLLET;
            break;
        default:
            conn_event |= EPOLLET;
            listen_event |= EPOLLET;
            break;
    }
    HttpConn::is_ET = (conn_event & EPOLLET);
}

/**
 * @brief 将指定文件描述符设为非阻塞的
 * @param fd 文件描述符
 * @return 原来的文件状态标志
*/
int WebServer::set_nonblocking(int fd) {
    assert(fd >= 0);
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

void WebServer::add_client(int fd, const sockaddr_in &addr) {
    assert(fd >= 0);
    users[fd].init(fd, addr);
    if (timeout > 0) {
        tm_heap->add(fd, timeout,
            std::bind(&WebServer::close_conn, this, &users[fd]));
    }
    epoller->add_fd(fd, conn_event | EPOLLIN);
    set_nonblocking(fd);
}

void WebServer::close_conn(HttpConn * client) {
    if (!client) return;
    epoller->del_fd(client->get_fd());
    client->close_conn();
}

void WebServer::extend_time(HttpConn *client) {
    if (timeout > 0) {
        tm_heap->adjust(client->get_fd(), timeout);
    }
}

void WebServer::send_error_msg(int fd, const char *msg) {
    assert(fd >= 0);
    int ret = send(fd, msg, strlen(msg), 0);
    if (ret < 0) {
        LOG_WARN("Failed to send error message to <client %d>!", fd);
    }
    close(fd);
}

void WebServer::deal_listen() {
    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);
    do {
        int fd = accept(listen_fd, (sockaddr*)&addr, &addr_len);
        if (fd < 0) {
            break;
        } else if (HttpConn::conn_count >= MAX_FD) {
            send_error_msg(fd, "Server busy!");
            LOG_WARN("Clients are full!");
            break;
        }
        add_client(fd, addr);
    } while (listen_event & EPOLLET);
}

void WebServer::deal_read(HttpConn *client) {
    assert(client);
    extend_time(client);
    thread_pool->add_task(std::bind(&WebServer::on_read, this, client));
}

void WebServer::on_read(HttpConn *client) {
    assert(client);
    int ret = -1, read_errno = 0;
    ret = client->read(&read_errno);
    if (ret <= 0 && read_errno != EAGAIN) {
        close_conn(client);
        return;
    }
    on_process(client);
}

void WebServer::on_process(HttpConn *client) {
    if (client->process()) {
        epoller->mod_fd(client->get_fd(), conn_event | EPOLLOUT);
    } else {
        epoller->mod_fd(client->get_fd(), conn_event | EPOLLIN);
    }
}

void WebServer::deal_write(HttpConn *client) {
    assert(client);
    extend_time(client);
    thread_pool->add_task(std::bind(&WebServer::on_write, this, client));
}

void WebServer::on_write(HttpConn *client) {
    assert(client);
    int ret = -1, write_errno = 0;
    ret = client->write(&write_errno);
    if (client->to_write_bytes() == 0) {
        if (client->is_keep_alive()) {
            on_process(client);
            return;
        }
    } else if (ret < 0) {
        if (write_errno == EAGAIN) {
            epoller->mod_fd(client->get_fd(), conn_event | EPOLLOUT);
            return;
        }
    }
    close_conn(client);
}