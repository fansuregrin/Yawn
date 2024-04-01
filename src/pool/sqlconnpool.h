/**
 * @file sqlconnpool.h
 * @author Fansure Grin
 * @date 2024-03-28
 * @brief header file for SQL connection pool
*/
#ifndef SQLCONNPOOL_H
#define SQLCONNPOOL_H

#include <queue>
#include <mutex>
#include <semaphore.h>
#include <mysql/mysql.h>


class SQLConnPool {
public:
    using lock_guard = std::lock_guard<std::mutex>;

    MYSQL* get_conn();
    void free_conn(MYSQL * conn);
    int get_free_conn_count();

    static SQLConnPool* get_instance();
    void init(const char *host, int port, const char *user, const char *passwd,
              const char *db_name, int conn_num = 8);
    void close();
private:
    SQLConnPool();
    ~SQLConnPool();

    int max_conn_num;
    int used_count;
    int free_count;
    std::queue<MYSQL*> conn_que;
    std::mutex mtx;
    sem_t sem_id;
};

#endif