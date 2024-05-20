cmake_minimum_required(VERSION 3.14)
project(yawn_test)

# GoogleTest requires at least C++14
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(GTest REQUIRED)

enable_testing()

add_executable(
  config_unittest
  config_unittest.cc
  ../src/config/config.cpp
  ../src/log/log.cpp
  ../src/buffer/buffer.cpp
)
add_executable(
  util_unittest
  util_unittest.cc
  ../src/util/util.cpp
)

target_link_libraries(
  config_unittest
  GTest::gtest_main
)
target_link_libraries(
  util_unittest
  GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(config_unittest)
gtest_discover_tests(util_unittest)

file(COPY test_server.cfg DESTINATION ${PROJECT_BINARY_DIR})