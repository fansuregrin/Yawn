/**
 * @file main.cpp
 * @author Fansure Grin
 * @date 2024-03-30
 * @brief the enter of this webserver project
*/
#include "server/webserver.h"


int main(int argc, char* argv[]) {
    // char src_dir[] = "/home/fansuregrin/workshop/my_http_server/resources";
    // WebServer server(
    //     "0.0.0.0",      // 监听的 IP 地址
    //     7777,           // 监听的端口号
    //     60000,          // 定时时间
    //     true,           // 开启 linger
    //     3,              // 监听socket 和 连接socket 上触发事件的模式
    //     "localhost",    // MySQL 的服务地址
    //     3306,           // MySQL 的端口号
    //     "mysql_user",   // MySQL 的账户名称
    //     "password",     // MySQL 的账户密码
    //     "webserver",    // 要连接的数据库名称
    //     2,              // MySQL 数据库连接池中的连接个数
    //     true,           // 是否开启日志
    //     1,              // 日志等级
    //     10,             // 日志模块中阻塞队列的大小
    //     "./logs",       // 存放日志文件的路径
    //     src_dir,        // 静态资源根目录
    //     2               // 线程池中线程的数量
    // );

    string cfg_fp("./server.cfg");
    if (argc > 1) {
        cfg_fp = argv[1];
    }

    Config cfg(cfg_fp);
    WebServer server(cfg);
    
    server.start();
}
