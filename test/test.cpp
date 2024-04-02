/**
 * @file test.cpp
 * @author Fansure Grin
 * @date 2024-03-31
 * @brief 测试入口
*/

void test_log();
void test_timer();
void test_config();
void test_threadpool(int thread_num, int task_num);

int main() {
    // test_log();
    // test_timer();
    // test_config();
    test_threadpool(4, 20);
}