cmake_minimum_required(VERSION 3.20)

enable_testing()

add_library(stdlib SHARED stdlib.cpp math.cpp strings.cpp functions.hpp)

include_directories(../include)

set_target_properties(stdlib PROPERTIES
    CXX_STANDARD 17
    CXX_EXTENSIONS OFF)

add_test(NAME Print1a COMMAND logical ../../stdlib/tests/print1.dl)
