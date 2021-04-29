
project (logical)
cmake_minimum_required(VERSION 3.20)

find_package(FLEX)
find_package(BISON)

LIST(APPEND CMAKE_FIND_LIBRARY_SUFFIXES ".a")
FIND_LIBRARY(FL_LIBRARY NAMES libfl fl DOC "/opt/homebrew/opt/flex/lib")

BISON_TARGET(MyParser logical.y ${CMAKE_CURRENT_BINARY_DIR}/parser.cpp DEFINES_FILE ${CMAKE_CURRENT_BINARY_DIR}/tokens.tab.h)
FLEX_TARGET(MyScanner logical.l  ${CMAKE_CURRENT_BINARY_DIR}/lexer.cpp)
ADD_FLEX_BISON_DEPENDENCY(MyScanner MyParser)
include_directories(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR})

add_library(logiccore 
    AST.cpp AST.hpp
    Database.cpp Database.hpp
    StringTable.cpp StringTable.hpp
    Entity.hpp
    Table.cpp Table.hpp
    Evaluation.cpp Evaluation.hpp
    Compiler.cpp Compiler.hpp
    Relation.cpp Relation.hpp
    Allocator.cpp Allocator.hpp
    ${BISON_MyParser_OUTPUTS}
    ${FLEX_MyScanner_OUTPUTS})

add_executable(logical main.cpp)

add_executable(logical-tests tests.cpp)
target_link_libraries(logical-tests logiccore)
target_link_libraries(logical logiccore)

set_target_properties(logiccore PROPERTIES
    CXX_STANDARD 17
    CXX_EXTENSIONS OFF)

set_target_properties(logical PROPERTIES
    CXX_STANDARD 17
    CXX_EXTENSIONS OFF)

set_target_properties(logical-tests PROPERTIES
    CXX_STANDARD 17
    CXX_EXTENSIONS OFF)

target_link_libraries(logiccore ${FLEX_LIBRARIES})    