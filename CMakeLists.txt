
project (logical)
cmake_minimum_required(VERSION 3.16)

enable_testing()

function(test_program DIRECTORY TESTNAME)
  add_test(NAME ${TESTNAME} COMMAND logical ${CMAKE_CURRENT_SOURCE_DIR}/${DIRECTORY}/${TESTNAME}.dl)
  add_test(NAME ${TESTNAME}-O0 COMMAND logical -O0 ${CMAKE_CURRENT_SOURCE_DIR}/${DIRECTORY}/${TESTNAME}.dl)
endfunction()

function(test_directory DIRECTORY)
  FILE(GLOB children RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/${DIRECTORY} ${CMAKE_CURRENT_SOURCE_DIR}/${DIRECTORY}/*.dl)
  FOREACH(TESTNAME ${children})
    add_test(NAME ${DIRECTORY}/${TESTNAME} WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${DIRECTORY} COMMAND logical ${CMAKE_CURRENT_SOURCE_DIR}/${DIRECTORY}/${TESTNAME})
    add_test(NAME ${DIRECTORY}/${TESTNAME}-O0 WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${DIRECTORY} COMMAND logical -O0 ${CMAKE_CURRENT_SOURCE_DIR}/${DIRECTORY}/${TESTNAME})
  ENDFOREACH()
endfunction()

add_subdirectory(src)
add_subdirectory(external/persist/src)
add_subdirectory(modules/stdlib)
add_subdirectory(modules/mysql)
add_subdirectory(tests/externs)
add_subdirectory(modules/parsers)

add_test(NAME UnitTests COMMAND logical-tests)

test_directory(tests/programs)
test_program(tests/mysql mysql)
test_directory(samples)
test_directory(tests/stdlib)
test_directory(tests/parsers)
test_directory(tests/externs)

