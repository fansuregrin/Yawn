/**
 * @file test_config.cpp
 * @author Fansure Grin
 * @date 2024-03-31
 * @brief config 模块的测试程序
*/
#include <iostream>
#include <cassert>
#include "../src/config/config.h"


void test_config() {
    Config cfg("./server.cfg");
    // std::cout << cfg;
    assert(cfg.items_num() == 17);
    assert(cfg.get_string("listen_ip")    == "0.0.0.0");
    assert(cfg.get_integer("listen_port") == 7777);
    assert(cfg.get_integer("timeout")     == 60000);
    assert(cfg.get_integer("trig_mode")   == 3);
    assert(cfg.get_bool("open_linger")    == true);
    assert(cfg.get_string("sql_host")     == "localhost");
    assert(cfg.get_integer("sql_port")    == 3306);
    assert(cfg.get_string("sql_username") == "rick");
    assert(cfg.get_string("sql_passwd")   == "rick123");
    assert(cfg.get_string("db_name")      == "webserver");
    assert(cfg.get_integer("conn_pool_num")  == 2);
    assert(cfg.get_bool("open_log")          == true);
    assert(cfg.get_integer("log_level")      == 1);
    assert(cfg.get_integer("log_queue_size") == 1000);
    assert(cfg.get_string("log_path") == "/tmp/webserver_logs");
    assert(cfg.get_string("src_dir")  == "/var/www/html");
    assert(cfg.get_integer("thread_pool_num") == 2);

    cfg.add("  ", "invalid key");
    assert(cfg.get_string("  ") == "");
    cfg.add("invalid val", "   ");
    assert(cfg.get_string("invalid val") == "");
    cfg.add("rick", "morty");
    assert(cfg.get_string("rick") == "morty");
    cfg.add("rick", "Mr. Meeseeks");
    assert(cfg.get_string("rick") == "morty");

    cfg.update("not exist", "hi");
    assert(cfg.get_string("not exist") == "");
    cfg.update("rick", "      ");
    assert(cfg.get_string("rick") == "morty");
    cfg.update("rick", "mooooorty");
    assert(cfg.get_string("rick") == "mooooorty");
}