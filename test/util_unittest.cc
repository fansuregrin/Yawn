/**
 * @file util_unittest.cc
 * @author Fansure Grin
 * @date 2024-05-20
 * @brief util 模块的测试程序
*/
#include <gtest/gtest.h>
#include "../src/util/util.h"

// 测试 dec2hexch
TEST(UtilTest, Dec2HexCh) {
    EXPECT_EQ(dec2hexch(0), '0');
    EXPECT_EQ(dec2hexch(1), '1');
    EXPECT_EQ(dec2hexch(2), '2');
    EXPECT_EQ(dec2hexch(3), '3');
    EXPECT_EQ(dec2hexch(4), '4');
    EXPECT_EQ(dec2hexch(5), '5');
    EXPECT_EQ(dec2hexch(6), '6');
    EXPECT_EQ(dec2hexch(7), '7');
    EXPECT_EQ(dec2hexch(8), '8');
    EXPECT_EQ(dec2hexch(9), '9');
    EXPECT_EQ(dec2hexch(10), 'a');
    EXPECT_EQ(dec2hexch(11), 'b');
    EXPECT_EQ(dec2hexch(12), 'c');
    EXPECT_EQ(dec2hexch(13), 'd');
    EXPECT_EQ(dec2hexch(14), 'e');
    EXPECT_EQ(dec2hexch(15), 'f');
}

// 测试 dec2hexstr
TEST(UtilTest, Dec2HexStr) {
    EXPECT_EQ(dec2hexstr(8862831), "873c6f");
    EXPECT_EQ(dec2hexstr(539352320), "2025dd00");
    EXPECT_EQ(dec2hexstr(257), "101");
}