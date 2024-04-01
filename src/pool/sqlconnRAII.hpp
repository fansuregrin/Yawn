/**
 * @file sqlconnRAII.hpp
 * @author Fansure Grin
 * @date 2024-03-28
 * @brief RAII(Resource Acqusition Is Initialization) for SQL-Connection
*/
#ifndef SQLCONNRAII_HPP
#define SQLCONNRAII_HPP

#include <cassert>
#include "sqlconnpool.h"

class SQLConnRAII {
public:
    SQLConnRAII(MYSQL* &conn_, SQLConnPool *conn_pool_) {
        assert(conn_pool_);
        conn_ = conn_pool_->get_conn();
        conn = conn_;
        conn_pool = conn_pool_;
    }

    ~SQLConnRAII() {
        if (conn && conn_pool) {
            conn_pool->free_conn(conn);
        }
    }
private:
    SQLConnPool * conn_pool;
    MYSQL * conn;
};

#endif