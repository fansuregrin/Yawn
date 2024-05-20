/**
 * @file test.cpp
 * @author Fansure Grin
 * @date 2024-03-31
 * @brief 测试入口
*/
#include <gtest/gtest.h>

int main(int argc, char* argv[]) {
    std::cout << "Running main() from " << __FILE__ << '\n';
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}