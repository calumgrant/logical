
project (logical)
cmake_minimum_required(VERSION 3.20)

enable_testing()

find_package(FLEX)
find_package(BISON)

LIST(APPEND CMAKE_FIND_LIBRARY_SUFFIXES ".a")
FIND_LIBRARY(FL_LIBRARY NAMES libfl fl DOC "/opt/homebrew/opt/flex/lib")

add_subdirectory(../external/persist/src persist)
add_subdirectory(../modules/stdlib stdlib)
add_subdirectory(../modules/mysql mysql)
add_subdirectory(../tests/externs externs)

BISON_TARGET(MyParser logical.y ${CMAKE_CURRENT_BINARY_DIR}/parser.cpp DEFINES_FILE ${CMAKE_CURRENT_BINARY_DIR}/tokens.tab.h)
FLEX_TARGET(MyScanner logical.l  ${CMAKE_CURRENT_BINARY_DIR}/lexer.cpp)
ADD_FLEX_BISON_DEPENDENCY(MyScanner MyParser)
include_directories(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/../external/persist/include ${CMAKE_CURRENT_SOURCE_DIR}/../external/simpletest ${CMAKE_CURRENT_SOURCE_DIR}/../include)

find_library(TERMCAP NAMES termcap)

add_library(logiccore SHARED
    Analysis.cpp Analysis.hpp OptimizerImpl.hpp
    AST.cpp AST.hpp
    Binary.hpp Binary.cpp
    Database.cpp Database.hpp DatabaseImpl.hpp
    StringTable.cpp StringTable.hpp
    Entity.hpp
    Colours.hpp
    Columns.hpp
    CompoundName.cpp CompoundName.hpp
    Externs.cpp
    Fwd.hpp
    Helpers.hpp
    ../include/Logical.hpp
    ../include/Program.hpp
    ../include/Utils.hpp
    ../include/HashTable.hpp
    ../include/Table.hpp
    ../include/Index.hpp
    ../include/HashIndex.hpp
    ../include/HashUtils.hpp
    Table.cpp Table.hpp TableImpl.hpp
    Evaluation.cpp Evaluation.hpp EvaluationImpl.hpp
    Compiler.cpp Compiler.hpp
    Relation.cpp Relation.hpp RelationImpl.hpp
    Allocator.cpp Allocator.hpp
    ${BISON_MyParser_OUTPUTS}
    ${FLEX_MyScanner_OUTPUTS})

add_executable(logical main.cpp)

add_executable(logical-tests ../tests/unittests/tests.cpp ../tests/unittests/tables.cpp)
target_link_libraries(logical-tests logiccore)
target_link_libraries(logical logiccore stdlib)
target_link_libraries(logiccore ${TERMCAP} persist)

add_executable(prime-generator primegenerator.cpp)

# add_custom_command(TARGET logical POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:mysql> $<TARGET_FILE_DIR:logical>)

set_target_properties(logiccore PROPERTIES
    CXX_STANDARD 17
    CXX_EXTENSIONS OFF)

    set_target_properties(prime-generator PROPERTIES
    CXX_STANDARD 17
    CXX_EXTENSIONS OFF)

set_target_properties(logical PROPERTIES
    CXX_STANDARD 17
    CXX_EXTENSIONS OFF)

set_target_properties(logical-tests PROPERTIES
    CXX_STANDARD 17
    CXX_EXTENSIONS OFF)

target_link_libraries(logiccore ${FLEX_LIBRARIES})

add_dependencies(logical mysql stdlib externs-test)

add_test(NAME UnitTests COMMAND logical-tests)

add_test(NAME Syntax COMMAND logical ../tests/programs/syntax.dl)
add_test(NAME Sourcecode COMMAND logical ../tests/programs/sourcecode.dl)
add_test(NAME tutorial COMMAND logical ../samples/tutorial.dl)

add_test(NAME Rule1 COMMAND logical ../tests/programs/rule1.dl)
add_test(NAME Rule2 COMMAND logical ../tests/programs/rule2.dl)
add_test(NAME Rule3 COMMAND logical ../tests/programs/rule3.dl)
add_test(NAME Rule4 COMMAND logical ../tests/programs/rule4.dl)
add_test(NAME Rule5 COMMAND logical ../tests/programs/rule5.dl)
add_test(NAME Rule6 COMMAND logical ../tests/programs/rule6.dl)
add_test(NAME Rule7 COMMAND logical ../tests/programs/rule7.dl)
add_test(NAME Rule8 COMMAND logical ../tests/programs/rule8.dl)

add_test(NAME Not1 COMMAND logical ../tests/programs/not1.dl)
add_test(NAME Not2 COMMAND logical ../tests/programs/not2.dl)

add_test(NAME IsNot1 COMMAND logical ../tests/programs/isnot1.dl)

add_test(NAME Or1 COMMAND logical ../tests/programs/or1.dl)
add_test(NAME Or2 COMMAND logical ../tests/programs/or2.dl)

add_test(NAME Print1 COMMAND logical ../tests/programs/print1.dl)
add_test(NAME Print2 COMMAND logical ../tests/programs/print2.dl)

add_test(NAME Exit1 COMMAND logical ../tests/programs/exit1.dl)
add_test(NAME Exit2 COMMAND logical ../tests/programs/exit2.dl)

add_test(NAME Binary1 COMMAND logical ../tests/programs/binary1.dl)
add_test(NAME Binary2 COMMAND logical ../tests/programs/binary2.dl)

add_test(NAME Range1 COMMAND logical ../tests/programs/range1.dl)
add_test(NAME Range2 COMMAND logical ../tests/programs/range2.dl)
add_test(NAME Range3 COMMAND logical ../tests/programs/range3.dl)

add_test(NAME Comparators1 COMMAND logical ../tests/programs/comparators1.dl)
add_test(NAME Arithmetic1 COMMAND logical ../tests/programs/arithmetic1.dl)
add_test(NAME Arithmetic2 COMMAND logical ../tests/programs/arithmetic2.dl)
add_test(NAME Arithmetic3 COMMAND logical ../tests/programs/arithmetic3.dl)
add_test(NAME Arithmetic4 COMMAND logical ../tests/programs/arithmetic4.dl)

add_test(NAME Count1 COMMAND logical ../tests/programs/count1.dl)
add_test(NAME Sum1 COMMAND logical ../tests/programs/sum1.dl)

add_test(NAME All1 COMMAND logical ../tests/programs/all1.dl)
add_test(NAME Primes1 COMMAND logical ../tests/programs/primes1.dl)

add_test(NAME Recursion1 COMMAND logical ../tests/programs/recursion1.dl)
add_test(NAME Recursion2 COMMAND logical ../tests/programs/recursion2.dl)
add_test(NAME Recursion3 COMMAND logical ../tests/programs/recursion3.dl)
add_test(NAME Recursion3a COMMAND logical -O0 ../tests/programs/recursion3.dl)
add_test(NAME Recursion4 COMMAND logical ../tests/programs/recursion4.dl)
add_test(NAME Recursion5 COMMAND logical ../tests/programs/recursion5.dl)
add_test(NAME Recursion6 COMMAND logical ../tests/programs/recursion6.dl)
add_test(NAME Recursion7 COMMAND logical ../tests/programs/recursion7.dl)

add_test(NAME Closure1 COMMAND logical ../tests/performance/closure1.dl)
add_test(NAME Closure2 COMMAND logical ../tests/performance/closure2.dl)
add_test(NAME Closure3 COMMAND logical ../tests/performance/closure3.dl)

add_test(NAME Projection1 COMMAND logical ../tests/programs/projection1.dl)
add_test(NAME Projection2 COMMAND logical ../tests/programs/projection2.dl)
add_test(NAME Projection3 COMMAND logical ../tests/programs/projection3.dl)
add_test(NAME Projection4 COMMAND logical ../tests/programs/projection4.dl)

add_test(NAME Query1 COMMAND logical ../tests/programs/query1.dl)

add_test(NAME New1 COMMAND logical ../tests/programs/new1.dl)
add_test(NAME New2 COMMAND logical ../tests/programs/new2.dl)
add_test(NAME New3 COMMAND logical ../tests/programs/new3.dl)
add_test(NAME New4 COMMAND logical ../tests/programs/new4.dl)
add_test(NAME New5 COMMAND logical ../tests/programs/new5.dl)

add_test(NAME Strlen1 COMMAND logical ../tests/programs/strlen1.dl)
add_test(NAME Strings1 COMMAND logical ../tests/programs/strings1.dl)

add_test(NAME Reaches1 COMMAND logical ../tests/programs/reaches1.dl)

add_test(NAME Datalog1 COMMAND logical ../tests/programs/datalog1.dl)
add_test(NAME Datalog2 COMMAND logical ../tests/programs/datalog2.dl)
add_test(NAME Datalog3 COMMAND logical ../tests/programs/datalog3.dl)
add_test(NAME Datalog4 COMMAND logical ../tests/programs/datalog4.dl)
add_test(NAME Datalog5 COMMAND logical ../tests/programs/datalog5.dl)
add_test(NAME Datalog6 COMMAND logical ../tests/programs/datalog6.dl)
