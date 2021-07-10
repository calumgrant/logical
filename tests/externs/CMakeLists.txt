cmake_minimum_required(VERSION 3.20)

enable_testing()

add_library(externs SHARED externs.cpp)

include_directories(../../include)

set_target_properties(externs PROPERTIES
    CXX_STANDARD 17
    CXX_EXTENSIONS OFF)

target_link_libraries(externs logiccore)

add_custom_command(TARGET externs POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:externs> $<TARGET_FILE_DIR:logical>)

# add_test(NAME Print1a COMMAND logical ../../tests/stdlib/print1.dl)
add_test(NAME LoadExterns COMMAND logical ../../tests/externs/load.dl)