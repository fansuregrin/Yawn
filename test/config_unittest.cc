/**
 * @file config_unittest.cc
 * @author Fansure Grin
 * @date 2024-05-19
 * @brief config 模块的测试程序
*/
#include <gtest/gtest.h>
#include "../src/config/config.h"


class ConfigTest: public testing::Test {
protected:
    ConfigTest(): cfg("./test_server.cfg") { }

    Config cfg;
};

// 测试从配置文件初始化配置对象
TEST_F(ConfigTest, InitFromConfigFile) {
    EXPECT_EQ(cfg.items_num(), 17);
    EXPECT_EQ(cfg.get_string("listen_ip"), "0.0.0.0");
    EXPECT_EQ(cfg.get_integer("timeout"), 60000);
    EXPECT_EQ(cfg.get_integer("trig_mode"), 3);
    EXPECT_EQ(cfg.get_bool("open_linger"), true);
    EXPECT_EQ(cfg.get_string("sql_host"), "localhost");
    EXPECT_EQ(cfg.get_integer("sql_port"), 3306);
    EXPECT_EQ(cfg.get_string("sql_username"), "rick");
    EXPECT_EQ(cfg.get_string("sql_passwd"), "rick123");
    EXPECT_EQ(cfg.get_string("db_name"), "webserver");
    EXPECT_EQ(cfg.get_integer("conn_pool_num"), 2);
    EXPECT_EQ(cfg.get_bool("open_log"), true);
    EXPECT_EQ(cfg.get_integer("log_level"), 1);
    EXPECT_EQ(cfg.get_integer("log_queue_size"), 1000);
    EXPECT_EQ(cfg.get_string("log_path"), "/tmp/webserver_logs");
    EXPECT_EQ(cfg.get_string("src_dir"), "/var/www/html");
    EXPECT_EQ(cfg.get_integer("thread_pool_num"), 2);
}

// 测试向配置对象中添加配置项
TEST_F(ConfigTest, AddItem) {
    cfg.add("  ", "invalid key");
    EXPECT_EQ(cfg.get_string("  "), "");
    cfg.add("invalid val", "   ");
    EXPECT_EQ(cfg.get_string("invalid val"), "");
    cfg.add("rick", "morty");
    EXPECT_EQ(cfg.get_string("rick"), "morty");
    cfg.add("rick", "Mr. Meeseeks");
    EXPECT_EQ(cfg.get_string("rick"), "morty");
}

// 测试更新配置对象中的配置项
TEST_F(ConfigTest, UpdateItem) {
    cfg.add("rick", "morty");
    cfg.update("not exist", "hi");
    EXPECT_EQ(cfg.get_string("not exist"), "");
    cfg.update("rick", "      ");
    EXPECT_EQ(cfg.get_string("rick"), "morty");
    cfg.update("rick", "mooooorty");
    EXPECT_EQ(cfg.get_string("rick"), "mooooorty");
}