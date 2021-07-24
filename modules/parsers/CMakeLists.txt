cmake_minimum_required(VERSION 3.20)

enable_testing()

add_library(parsers SHARED parsers.cpp)

find_library(ANTLR4_LIB NAMES "libantlr4-runtime.a" HINTS "/opt/homebrew/lib")
find_path( ANTLR_INCLUDE_DIR NAMES "antlr4-runtime.h" HINTS "/opt/homebrew/include/antlr4-runtime")

include_directories(../../include ${ANTLR_INCLUDE_DIR})

set_target_properties(parsers PROPERTIES
    CXX_STANDARD 17
    CXX_EXTENSIONS OFF)

target_link_libraries(parsers logiccore ${ANTLR4_LIB})

add_test(NAME LoadModule COMMAND logical ../../../tests/parsers/loadmodule.dl)
#add_test(NAME Connect COMMAND logical ../../tests/mysql/connection1.dl)
#add_test(NAME Execute COMMAND logical ../../tests/mysql/execute1.dl)
#add_test(NAME Sync COMMAND logical ../../tests/mysql/sync1.dl)

add_custom_command(TARGET parsers POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:parsers> $<TARGET_FILE_DIR:logical>)

