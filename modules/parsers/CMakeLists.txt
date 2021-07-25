cmake_minimum_required(VERSION 3.20)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

# compiler must be 11 or 14
set(CMAKE_CXX_STANDARD 17)

# required if linking to static library
add_definitions(-DANTLR4CPP_STATIC)

# using /MD flag for antlr4_runtime (for Visual C++ compilers only)
set(ANTLR4_WITH_STATIC_CRT OFF)
# add external build for antlrcpp
include(ExternalAntlr4Cpp)
# add antrl4cpp artifacts to project environment
include_directories(${ANTLR4_INCLUDE_DIRS})

# set variable pointing to the antlr tool that supports C++
# this is not required if the jar file can be found under PATH environment
set(ANTLR_EXECUTABLE /opt/homebrew/opt/antlr/antlr-4.9.2-complete.jar)
# add macros to generate ANTLR Cpp code from grammar
find_package(ANTLR REQUIRED)

# Call macro to add lexer and grammar to your build dependencies.
antlr_target(JavaLexer JavaLexer.g4 LEXER
             PACKAGE antlrcpptest)
antlr_target(JavaParser JavaParser.g4 PARSER
             PACKAGE antlrcpptest
             DEPENDS_ANTLR JavaLexer
             COMPILE_FLAGS -lib ${ANTLR_JavaLexer_OUTPUT_DIR})

# include generated files in project environment
include_directories(${ANTLR_JavaLexer_OUTPUT_DIR})
include_directories(${ANTLR_JavaParser_OUTPUT_DIR})


enable_testing()

add_library(parsers SHARED parsers.cpp ${ANTLR_JavaLexer_CXX_OUTPUTS} ${ANTLR_JavaParser_CXX_OUTPUTS})

#find_library(ANTLR4_LIB NAMES "libantlr4-runtime.a" HINTS "/opt/homebrew/lib")
#find_path( ANTLR_INCLUDE_DIR NAMES "antlr4-runtime.h" HINTS "/opt/homebrew/include/antlr4-runtime")

include_directories(../../include)

set_target_properties(parsers PROPERTIES
    CXX_STANDARD 17
    CXX_EXTENSIONS OFF)

target_link_libraries(parsers logiccore antlr4_static)

add_test(NAME LoadModule COMMAND logical ../../../tests/parsers/loadmodule.dl)
#add_test(NAME Connect COMMAND logical ../../tests/mysql/connection1.dl)
#add_test(NAME Execute COMMAND logical ../../tests/mysql/execute1.dl)
#add_test(NAME Sync COMMAND logical ../../tests/mysql/sync1.dl)

add_custom_command(TARGET parsers POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:parsers> $<TARGET_FILE_DIR:logical>)
