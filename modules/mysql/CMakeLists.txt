cmake_minimum_required(VERSION 3.20)

enable_testing()

add_library(mysql SHARED mysql.cpp)

find_library(MYSQLLIB mysqllib NAMES "mysqlclient" "mysqlclient_r")
find_path( MYSQL_INCLUDE_DIR NAMES "mysql.h" HINTS "/opt/homebrew/include")

include_directories(../../include ${MYSQL_INCLUDE_DIR})

set_target_properties(mysql PROPERTIES
    CXX_STANDARD 17
    CXX_EXTENSIONS OFF)

target_link_libraries(mysql logiccore ${MYSQLLIB})

add_test(NAME LoadMySqlModule COMMAND logical ../../tests/mysql/loadmodule.dl)
add_test(NAME Connect COMMAND logical ../../tests/mysql/connection1.dl)
add_test(NAME Execute COMMAND logical ../../tests/mysql/execute1.dl)
add_test(NAME Sync COMMAND logical ../../tests/mysql/sync1.dl)

add_custom_command(TARGET mysql POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:mysql> $<TARGET_FILE_DIR:logical>)

