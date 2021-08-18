cmake_minimum_required(VERSION 3.16)

enable_testing()

add_library(externs-test SHARED externs.cpp)

include_directories(../../include)

set_target_properties(externs-test PROPERTIES
    CXX_STANDARD 17
    CXX_EXTENSIONS OFF)

target_link_libraries(externs-test logiccore)

# add_test(NAME Print1a COMMAND logical ../../tests/stdlib/print1.dl)
add_test(NAME LoadExterns COMMAND logical ../../../tests/externs/load.dl)
add_test(NAME CountArgs COMMAND logical ../../../tests/externs/count.dl)
add_test(NAME ListArgs COMMAND logical ../../../tests/externs/listargs.dl)
add_test(NAME SetData COMMAND logical ../../../tests/externs/setdata.dl)
add_test(NAME Table COMMAND logical ../../../tests/externs/table.dl)