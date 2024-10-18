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
    const char *sql_host, int sql_port,
    const char *sql_username, const char *sql_passwd,
    const char *db_name, int conn_pool_num
) {
    SQLConnPool::get_instance()->init(sql_host, sql_port, sql_username,
        sql_passwd, db_name, conn_pool_num);
    LOG_INFO("Number of connections in SQL-Pool: %d", conn_pool_num);
}

WebServer::WebServer(const Config &cfg):
m_is_close(false), m_tm_heap(new TimeHeap()), m_epoller(new Epoller()) {
    LOG_INFO("====== Server initialization ======");

    // 初始化 socket
    if (!init_socket(
        cfg.get_string("listen_ip"),
        cfg.get_integer("listen_port"),
        cfg.get_integer("timeout"),
        cfg.get_bool("open_linger"),
        cfg.get_integer("trig_mode")
    )) {
        m_is_close = true;
    }

    m_src_dir = cfg.get_string("src_dir");
    LOG_INFO("Resource directory: %s", m_src_dir.c_str());
    auto thread_count = cfg.get_integer("thread_pool_num");
    m_thread_pool.reset(new ThreadPool(thread_count));
    LOG_INFO("Number of threads in Thread-Pool: %d", thread_count);

    HttpConn::conn_count = 0;
    HttpConn::src_dir = m_src_dir;

    // 初始化数据库连接池
    m_enable_db = cfg.get_bool("enable_db");
    if (m_enable_db) {
        init_db_pool(
            cfg.get_string("sql_host").c_str(),
            cfg.get_integer("sql_port"),
            cfg.get_string("sql_username").c_str(),
            cfg.get_string("sql_passwd").c_str(),
            cfg.get_string("db_name").c_str(),
            cfg.get_integer("conn_pool_num"));
    }

    if (m_is_close) {
        LOG_ERROR("Server initialization error");
    }
}

WebServer::~WebServer() {
    close(m_listen_fd);
    m_is_close = true;
    if (m_enable_db) {
        SQLConnPool::get_instance()->close();
    }
    LOG_INFO("====== Server closed ======");
}

void WebServer::start() {
    if (m_is_close) return;
    LOG_INFO("====== Server start ======");
    int wait_tm = -1;
    while (!m_is_close) {
        if (m_timeout > 0) {
            // 处理定时事件
            wait_tm = m_tm_heap->get_next_tick();
        }
        int event_cnt = m_epoller->wait(wait_tm);
        for (int i=0; i<event_cnt; ++i) {
            int fd = m_epoller->get_event_fd(i);
            uint32_t events = m_epoller->get_events(i);
            if (fd == m_listen_fd) {
                deal_listen();
            } else if (events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
                if (m_clients.count(fd)) {
                    close_conn(m_clients[fd]);
                }
            } else if (events & EPOLLIN) {
                if (m_clients.count(fd)) {
                    deal_read(m_clients[fd]);
                }
            } else if (events & EPOLLOUT) {
                if (m_clients.count(fd)) {
                    deal_write(m_clients[fd]);
                }
            } else {
                LOG_ERROR("Unexpected event!");
            }
        }
    }
}

bool WebServer::init_socket(
    const string &ip, int listen_port, int timeout,
    bool open_linger, int trig_mode
) {
    m_ip = ip;
    m_listen_port = listen_port;
    m_timeout = timeout;
    m_open_linger = open_linger;
    init_event_mode(trig_mode);

    int ret;
    
    struct sockaddr_in addr;
    if (m_listen_port > 65535 || m_listen_port < 1024) {
        LOG_ERROR("Invalid port number: %d (1024 <= port <= 65535)", m_listen_port);
        return false;
    }
    addr.sin_family = AF_INET;
    inet_pton(AF_INET, m_ip.c_str(), &addr.sin_addr);
    addr.sin_port = htons(m_listen_port);

    m_listen_fd = socket(PF_INET, SOCK_STREAM, 0);
    if (m_listen_fd < 0) {
        LOG_ERROR("Create socket error!");
        return false;
    }
    struct linger opt_linger = {0, 0};
    if (m_open_linger) {
        // 开启linger选项：如有数据待发送，则延迟关闭
        opt_linger.l_onoff = 1;
        opt_linger.l_linger = 1;
    }
    ret = setsockopt(m_listen_fd, SOL_SOCKET, SO_LINGER, &opt_linger,
        sizeof(opt_linger));
    if (ret < 0) {
        close(m_listen_fd);
        LOG_ERROR("Set linger error!");
        return false;
    }
    int optval = 1;
    // 设置地址重用
    ret = setsockopt(m_listen_fd, SOL_SOCKET, SO_REUSEADDR, &optval,
        sizeof(optval));
    if (ret < 0) {
        close(m_listen_fd);
        LOG_ERROR("Set reuse-address error!");
        return false;
    }

    ret = bind(m_listen_fd, (sockaddr *)&addr, sizeof(addr));
    if (ret < 0) {
        close(m_listen_fd);
        LOG_ERROR("Bind %s:%d error!", m_ip, m_listen_port);
        return false;
    }

    ret = listen(m_listen_fd, 6);
    if (ret < 0) {
        close(m_listen_fd);
        LOG_ERROR("Listen %s:%d error!", m_ip, m_listen_port);
        return false;
    }

    ret = m_epoller->add_fd(m_listen_fd, m_listen_event | EPOLLIN);
    if (ret < 0) {
        close(m_listen_fd);
        LOG_ERROR("Add listen events error!");
        return false;
    }

    set_nonblocking(m_listen_fd);
    LOG_INFO("Listen on %s:%d, open-linger: %s", m_ip.c_str(), m_listen_port,
            (m_open_linger ? "true" : "false"));
    LOG_INFO("Listen mode: %s, Open connection mode: %s",
        ((m_listen_event & EPOLLET) ? "ET" : "LT"),
        ((m_conn_event & EPOLLET) ? "ET" : "LT"));
    return true;
}

void WebServer::init_event_mode(int trig_mode) {
    m_listen_event = EPOLLRDHUP;
    m_conn_event = EPOLLONESHOT | EPOLLRDHUP;
    switch (trig_mode) {
        case 0: break;
        case 1:
            m_conn_event |= EPOLLET;
            break;
        case 2:
            m_listen_event |= EPOLLET;
            break;
        case 3:
            m_conn_event |= EPOLLET;
            m_listen_event |= EPOLLET;
            break;
        default:
            m_conn_event |= EPOLLET;
            m_listen_event |= EPOLLET;
            break;
    }
    HttpConn::is_ET = (m_conn_event & EPOLLET);
}

/**
 * @brief 将指定文件描述符设为非阻塞的
 * @param fd 文件描述符
 * @return 原来的文件状态标志
*/
int WebServer::set_nonblocking(int fd) {
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

void WebServer::add_client(int fd, const sockaddr_in &addr) {
    if (fd < 0) return;
    m_clients[fd] = std::make_shared<HttpConn>();
    m_clients[fd]->init(fd, addr);
    if (m_timeout > 0) {
        m_tm_heap->add(fd, m_timeout,
            std::bind(&WebServer::close_conn, this, m_clients[fd]));
    }
    m_epoller->add_fd(fd, m_conn_event | EPOLLIN);
    set_nonblocking(fd);
}

void WebServer::close_conn(std::shared_ptr<HttpConn> client) {
    if (!client) return;
    m_epoller->del_fd(client->get_fd());
    client->close_conn();
}

void WebServer::extend_time(std::shared_ptr<HttpConn> client) {
    if (m_timeout > 0) {
        m_tm_heap->adjust(client->get_fd(), m_timeout);
    }
}

void WebServer::send_error_msg(int fd, const char *msg) {
    if (fd < 0) return;
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
        int fd = accept(m_listen_fd, (sockaddr*)&addr, &addr_len);
        if (fd < 0) {
            break;
        } else if (HttpConn::conn_count >= MAX_FD) {
            send_error_msg(fd, "Server busy!");
            LOG_WARN("Clients are full!");
            break;
        }
        add_client(fd, addr);
    } while (m_listen_event & EPOLLET);
}

void WebServer::deal_read(std::shared_ptr<HttpConn> client) {
    if (!client) return;
    extend_time(client);
    m_thread_pool->add_task(std::bind(&WebServer::on_read, this, client));
}

void WebServer::on_read(std::shared_ptr<HttpConn> client) {
    if (!client) return;
    int ret = -1, read_errno = 0;
    ret = client->read(&read_errno);
    if (ret <= 0 && read_errno != EAGAIN) {
        close_conn(client);
        return;
    }
    on_process(client);
}

void WebServer::on_process(std::shared_ptr<HttpConn> client) {
    if (client->process()) {
        m_epoller->mod_fd(client->get_fd(), m_conn_event | EPOLLOUT);
    } else {
        m_epoller->mod_fd(client->get_fd(), m_conn_event | EPOLLIN);
    }
}

void WebServer::deal_write(std::shared_ptr<HttpConn> client) {
    if (!client) return;
    extend_time(client);
    m_thread_pool->add_task(std::bind(&WebServer::on_write, this, client));
}

void WebServer::on_write(std::shared_ptr<HttpConn> client) {
    if(!client) return;
    int ret = -1, write_errno = 0;
    ret = client->write(&write_errno);
    if (client->to_write_bytes() == 0) {
        if (client->is_keep_alive()) {
            on_process(client);
            return;
        }
    } else if (ret < 0) {
        if (write_errno == EAGAIN) {
            m_epoller->mod_fd(client->get_fd(), m_conn_event | EPOLLOUT);
            return;
        }
    }
    close_conn(client);
}