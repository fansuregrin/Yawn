/**
 * @file sqlconnpool.cpp
 * @author Fansure Grin
 * @date 2024-03-28
 * @brief source file for SQL connection pool
*/
#include <cassert>
#include "sqlconnpool.h"
#include "../log/log.h"


MYSQL* SQLConnPool::get_conn() {
    MYSQL * conn = nullptr;
    if (conn_que.empty()) {
        LOG_WARN("SQL-Connection-Pool busy!");
        return nullptr;
    }
    sem_wait(&sem_id);
    {
        lock_guard lck(mtx);
        conn = conn_que.front();
        conn_que.pop();
    }
    return conn;
}

void SQLConnPool::free_conn(MYSQL * conn) {
    assert(conn);
    lock_guard lck(mtx);
    conn_que.push(conn);
    sem_post(&sem_id);
}

int SQLConnPool::get_free_conn_count() {
    lock_guard lck(mtx);
    return conn_que.size();
}

SQLConnPool* SQLConnPool::get_instance() {
    static SQLConnPool pool;
    return &pool;
}

void SQLConnPool::init(const char *host, int port, const char *user,
const char *passwd, const char *db_name, int conn_num) {
    assert(conn_num > 0);
    for (int i=0; i<conn_num; ++i) {
        MYSQL * conn = nullptr;
        conn = mysql_init(conn);
        if (!conn) {
            LOG_ERROR("MySQL initialization error!");
            assert(conn);
        }
        conn = mysql_real_connect(conn, host, user, passwd, db_name, port,
            nullptr, 0);
        if (!conn) {
            LOG_ERROR("MySQL connection error!");
            assert(conn);
        }
        conn_que.push(conn);
    }
    LOG_INFO("The SQL-Connection-Pool was successfully initialized, with a"
        " total of eight connections in the pool.");
    max_conn_num = conn_num;
    sem_init(&sem_id, 0, max_conn_num);
}

void SQLConnPool::close() {
    lock_guard lck(mtx);
    while (!conn_que.empty()) {
        auto conn = conn_que.front();
        conn_que.pop();
        mysql_close(conn);
    }
    mysql_library_end();
}

SQLConnPool::SQLConnPool(): used_count(0), free_count(0) {}

SQLConnPool::~SQLConnPool() {
    close();
}
