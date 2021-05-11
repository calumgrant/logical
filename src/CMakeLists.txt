
project (logical)
cmake_minimum_required(VERSION 3.20)

enable_testing()

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
    Binary.hpp Binary.cpp
    Database.cpp Database.hpp
    StringTable.cpp StringTable.hpp
    Entity.hpp
    CompoundName.cpp CompoundName.hpp
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

add_test(NAME UnitTests COMMAND logical-tests)

add_test(NAME Syntax COMMAND logical ../tests/syntax.dl)
add_test(NAME Sourcecode COMMAND logical ../tests/sourcecode.dl)
add_test(NAME tutorial COMMAND logical ../samples/tutorial.dl)

add_test(NAME Rule1 COMMAND logical ../tests/rule1.dl)
add_test(NAME Rule2 COMMAND logical ../tests/rule2.dl)
add_test(NAME Rule3 COMMAND logical ../tests/rule3.dl)
add_test(NAME Rule4 COMMAND logical ../tests/rule4.dl)
add_test(NAME Rule5 COMMAND logical ../tests/rule5.dl)
add_test(NAME Rule6 COMMAND logical ../tests/rule6.dl)
add_test(NAME Rule7 COMMAND logical ../tests/rule7.dl)

add_test(NAME Not1 COMMAND logical ../tests/not1.dl)
add_test(NAME Not2 COMMAND logical ../tests/not2.dl)

add_test(NAME Or1 COMMAND logical ../tests/or1.dl)
add_test(NAME Or2 COMMAND logical ../tests/or2.dl)

add_test(NAME Print1 COMMAND logical ../tests/print1.dl)
add_test(NAME Print2 COMMAND logical ../tests/print2.dl)

add_test(NAME Exit1 COMMAND logical ../tests/exit1.dl)
add_test(NAME Exit2 COMMAND logical ../tests/exit2.dl)

add_test(NAME Binary1 COMMAND logical ../tests/binary1.dl)
add_test(NAME Binary2 COMMAND logical ../tests/binary2.dl)

add_test(NAME Range1 COMMAND logical ../tests/range1.dl)
add_test(NAME Comparators1 COMMAND logical ../tests/comparators1.dl)
add_test(NAME Arithmetic1 COMMAND logical ../tests/arithmetic1.dl)
add_test(NAME Arithmetic2 COMMAND logical ../tests/arithmetic2.dl)
add_test(NAME Arithmetic3 COMMAND logical ../tests/arithmetic3.dl)
add_test(NAME Arithmetic4 COMMAND logical ../tests/arithmetic4.dl)

add_test(NAME Count1 COMMAND logical ../tests/count1.dl)
add_test(NAME Sum1 COMMAND logical ../tests/sum1.dl)

add_test(NAME All1 COMMAND logical ../tests/all1.dl)
add_test(NAME Primes1 COMMAND logical ../tests/primes1.dl)
