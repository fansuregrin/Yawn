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

// 测试 http_gmt(time_t)
TEST(UtilTest, HttpGmt) {
    EXPECT_EQ(http_gmt(1716214212), "Mon, 20 May 2024 14:10:12 GMT");
}

// 测试 hexch2dec
TEST(UtilTest, HexCh2Dec) {
    EXPECT_EQ(hexch2dec('0'), 0);
    EXPECT_EQ(hexch2dec('1'), 1);
    EXPECT_EQ(hexch2dec('2'), 2);
    EXPECT_EQ(hexch2dec('3'), 3);
    EXPECT_EQ(hexch2dec('4'), 4);
    EXPECT_EQ(hexch2dec('5'), 5);
    EXPECT_EQ(hexch2dec('6'), 6);
    EXPECT_EQ(hexch2dec('7'), 7);
    EXPECT_EQ(hexch2dec('8'), 8);
    EXPECT_EQ(hexch2dec('9'), 9);
    EXPECT_EQ(hexch2dec('a'), 10);
    EXPECT_EQ(hexch2dec('b'), 11);
    EXPECT_EQ(hexch2dec('c'), 12);
    EXPECT_EQ(hexch2dec('d'), 13);
    EXPECT_EQ(hexch2dec('e'), 14);
    EXPECT_EQ(hexch2dec('f'), 15);
    EXPECT_EQ(hexch2dec('A'), 10);
    EXPECT_EQ(hexch2dec('B'), 11);
    EXPECT_EQ(hexch2dec('C'), 12);
    EXPECT_EQ(hexch2dec('D'), 13);
    EXPECT_EQ(hexch2dec('E'), 14);
    EXPECT_EQ(hexch2dec('F'), 15);
}

// 测试 str_lower
TEST(UtilTest, StrLower) {
    EXPECT_EQ(str_lower("I'm fine"), "i'm fine");
    EXPECT_EQ(str_lower("User-Agent"), "user-agent");
    EXPECT_EQ(str_lower("我 LiKE 橘子"), "我 like 橘子");
}