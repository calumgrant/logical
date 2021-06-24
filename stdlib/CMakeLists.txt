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
add_test(NAME Strings1 COMMAND logical ../../stdlib/tests/strings1.dl)
add_test(NAME Strings2 COMMAND logical ../../stdlib/tests/strings2.dl)

add_test(NAME File1 COMMAND logical ../../stdlib/tests/file1.dl)
add_test(NAME File2 COMMAND logical ../../stdlib/tests/file2.dl)
add_test(NAME Regex1 COMMAND logical ../../stdlib/tests/regex1.dl)

add_test(NAME Random COMMAND logical ../../stdlib/tests/random.dl)
add_test(NAME Environment COMMAND logical ../../stdlib/tests/environment.dl)
add_test(NAME MessageOfTheDay COMMAND logical ../../stdlib/tests/messages.dl)
