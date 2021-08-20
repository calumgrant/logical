cmake_minimum_required(VERSION 3.16)

enable_testing()

add_library(mysql SHARED mysql.cpp)

find_library(MYSQLLIB mysqllib NAMES "mysqlclient" "mysqlclient_r")
find_path( MYSQL_INCLUDE_DIR NAMES "mysql.h" HINTS "/opt/homebrew/include" "/usr/include/mysql")

include_directories(../../include ${MYSQL_INCLUDE_DIR})

set_target_properties(mysql PROPERTIES
    CXX_STANDARD 17
    CXX_EXTENSIONS OFF)

target_link_libraries(mysql logiccore ${MYSQLLIB})

add_test(NAME MySql COMMAND logical ../../../tests/mysql/mysql.dl)
