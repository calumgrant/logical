cmake_minimum_required(VERSION 3.20)

enable_testing()

add_library(parsers SHARED parsers.cpp)

#find_library(MYSQLLIB mysqllib NAMES "mysqlclient" "mysqlclient_r")
# find_path( MYSQL_INCLUDE_DIR NAMES "mysql.h" HINTS "/opt/homebrew/include")

include_directories(../../include)

set_target_properties(parsers PROPERTIES
    CXX_STANDARD 17
    CXX_EXTENSIONS OFF)

target_link_libraries(parsers logiccore)

add_test(NAME LoadModule COMMAND logical ../../../tests/parsers/loadmodule.dl)
#add_test(NAME Connect COMMAND logical ../../tests/mysql/connection1.dl)
#add_test(NAME Execute COMMAND logical ../../tests/mysql/execute1.dl)
#add_test(NAME Sync COMMAND logical ../../tests/mysql/sync1.dl)

add_custom_command(TARGET parsers POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:parsers> $<TARGET_FILE_DIR:logical>)

