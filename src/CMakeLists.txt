
project (logical)
cmake_minimum_required(VERSION 3.16)

enable_testing()

# FIND_LIBRARY(FL_LIBRARY NAMES libfl fl DOC "/opt/homebrew/opt/flex/lib")

find_package(FLEX)
find_package(BISON)

LIST(APPEND CMAKE_FIND_LIBRARY_SUFFIXES ".a")
# FIND_LIBRARY(FL_LIBRARY NAMES libfl fl DOC HINTS "/opt/homebrew/opt/flex/lib" )

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
    SourceLocation.hpp
    PredicateName.hpp
    Relation.cpp Relation.hpp RelationImpl.hpp
    Allocator.cpp Allocator.hpp
    ../include/TableWriter.hpp
    TableWriter.cpp
    ${BISON_MyParser_OUTPUTS}
    ${FLEX_MyScanner_OUTPUTS})

add_executable(logical main.cpp)

add_custom_command(TARGET logical POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:stdlib> $<TARGET_FILE_DIR:logical>)
add_custom_command(TARGET logical POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:mysql> $<TARGET_FILE_DIR:logical>)
add_custom_command(TARGET logical POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:externs-test> $<TARGET_FILE_DIR:logical>)
add_custom_command(TARGET logical POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:parsers> $<TARGET_FILE_DIR:logical>)

add_executable(logical-tests ../tests/unittests/tests.cpp ../tests/unittests/tables.cpp)
target_link_libraries(logical-tests logiccore)
target_link_libraries(logical logiccore stdlib)
target_link_libraries(logiccore ${TERMCAP} persist)

add_executable(prime-generator primegenerator.cpp)

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

target_link_libraries(logiccore dl)

add_dependencies(logical mysql stdlib externs-test parsers)
