cmake_minimum_required(VERSION 3.20)

enable_testing()

add_library(mysql SHARED mysql.cpp)

include_directories(../../include)

set_target_properties(mysql PROPERTIES
    CXX_STANDARD 17
    CXX_EXTENSIONS OFF)

target_link_libraries(mysql logiccore)

add_test(NAME LoadMySqlModule COMMAND logical ../../tests/mysql/loadmodule.dl)

