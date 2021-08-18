cmake_minimum_required(VERSION 3.16)

enable_testing()

add_library(stdlib SHARED stdlib.cpp math.cpp strings.cpp functions.hpp)

include_directories(../../include)

set_target_properties(stdlib PROPERTIES
    CXX_STANDARD 17
    CXX_EXTENSIONS OFF)

target_link_libraries(stdlib logiccore)

add_test(NAME Print1a COMMAND logical ../../../tests/stdlib/print1.dl)
add_test(NAME Pi1 COMMAND logical ../../../tests/stdlib/pi1.dl)
add_test(NAME Pi2 COMMAND logical ../../../tests/stdlib/pi2.dl)
add_test(NAME Strings1 COMMAND logical ../../../tests/stdlib/strings1.dl)
add_test(NAME Strings2 COMMAND logical ../../../tests/stdlib/strings2.dl)

add_test(NAME Types COMMAND logical ../../../tests/stdlib/types.dl)
add_test(NAME Errors COMMAND logical ../../../tests/stdlib/error1.dl)

add_test(NAME File1 COMMAND logical ../../../tests/stdlib/file1.dl)
add_test(NAME File2 COMMAND logical ../../../tests/stdlib/file2.dl)
add_test(NAME Regex1 COMMAND logical ../../../tests/stdlib/regex1.dl)

add_test(NAME Random COMMAND logical ../../../tests/stdlib/random.dl)
add_test(NAME Environment COMMAND logical ../../../tests/stdlib/environment.dl)
add_test(NAME MessageOfTheDay COMMAND logical ../../../tests/stdlib/messages.dl)
add_test(NAME CSV COMMAND logical ../../../tests/stdlib/csv.dl)
