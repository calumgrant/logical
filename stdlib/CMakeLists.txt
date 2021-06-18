cmake_minimum_required(VERSION 3.20)

enable_testing()

add_library(stdlib SHARED stdlib.cpp math.cpp strings.cpp functions.hpp)

include_directories(../include)

set_target_properties(stdlib PROPERTIES
    CXX_STANDARD 17
    CXX_EXTENSIONS OFF)

target_link_libraries(stdlib logiccore)

add_test(NAME Print1a COMMAND logical ../../stdlib/tests/print1.dl)
add_test(NAME Pi1 COMMAND logical ../../stdlib/tests/pi1.dl)
add_test(NAME Pi2 COMMAND logical ../../stdlib/tests/pi2.dl)
