cmake_minimum_required(VERSION 3.14)
project(yawn VERSION 0.0.0)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_library(mysqlclient Names mysqlclient REQUIRED)
find_library(pthread Names pthread REQUIRED)

set(PROJECT_SOURCE_DIR ${PROJECT_SOURCE_DIR}/src)
configure_file(
    ${PROJECT_SOURCE_DIR}/version.h.in 
    ${PROJECT_SOURCE_DIR}/version.h
)

add_executable(
    yawn
    ${PROJECT_SOURCE_DIR}/main.cpp
    ${PROJECT_SOURCE_DIR}/buffer/buffer.cpp
    ${PROJECT_SOURCE_DIR}/log/log.cpp
    ${PROJECT_SOURCE_DIR}/epoller/epoller.cpp
    ${PROJECT_SOURCE_DIR}/timer/heap_timer.cpp
    ${PROJECT_SOURCE_DIR}/pool/sqlconnpool.cpp
    ${PROJECT_SOURCE_DIR}/http/httprequest.cpp
    ${PROJECT_SOURCE_DIR}/http/httpresponse.cpp
    ${PROJECT_SOURCE_DIR}/http/httpconn.cpp
    ${PROJECT_SOURCE_DIR}/config/config.cpp
    ${PROJECT_SOURCE_DIR}/server/webserver.cpp
)
target_link_libraries(
    yawn
    pthread
    mysqlclient
)