/**
 * @file test_log.cpp
 * @author Fansure Grin
 * @date 2024-03-26
 * @brief log模块的测试程序
*/
#include <string>
#include <cstring>
#include <unistd.h>
#include "../src/log/log.h"


void test_log() {
    using level_type = Log::level_type;
    char cwd_[512];
    memset(cwd_, '\0', sizeof(cwd_));
    getcwd(cwd_, sizeof(cwd_));
    std::string cwd(cwd_);
    int cnt = 0;
    Log::get_instance()->init(1, cwd+"/test_log1", ".log", 0);
    for (level_type lv=3; lv>=0; --lv) {
        Log::get_instance()->set_level(lv);
        for (int i=0; i<10000; ++i) {
            for (level_type new_lv=0; new_lv<=3; ++new_lv) {
                LOG_BASE(new_lv, "%s ===== %d", "test", cnt++);
            }
        }
    }
    Log::get_instance()->init(1, cwd+"/test_log2", ".log", 5000);
    cnt = 0;
    for (level_type lv=0; lv<=3; ++lv) {
        Log::get_instance()->set_level(lv);
        for (int i=0; i<10000; ++i) {
            for (level_type new_lv=3; new_lv>=0; --new_lv) {
                LOG_BASE(new_lv, "%s ==== %d", "test", cnt++);
            }
        }
    }
}