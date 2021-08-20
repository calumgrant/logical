cmake_minimum_required(VERSION 3.16)

add_library(stdlib SHARED stdlib.cpp math.cpp strings.cpp functions.hpp)

include_directories(../../include)

set_target_properties(stdlib PROPERTIES
    CXX_STANDARD 17
    CXX_EXTENSIONS OFF)

target_link_libraries(stdlib logiccore)
