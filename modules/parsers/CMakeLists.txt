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
             PACKAGE java)
antlr_target(JavaParser JavaParser.g4 PARSER
             PACKAGE java
             DEPENDS_ANTLR JavaLexer
             COMPILE_FLAGS -lib ${ANTLR_JavaLexer_OUTPUT_DIR})

# include generated files in project environment
include_directories(${ANTLR_JavaLexer_OUTPUT_DIR} ${ANTLR_JavaParser_OUTPUT_DIR})


antlr_target(JavaScriptLexer JavaScriptLexer.g4 LEXER
             PACKAGE javascript)
antlr_target(JavaScriptParser JavaScriptParser.g4 PARSER
             PACKAGE javascript
             DEPENDS_ANTLR JavaScriptLexer
             COMPILE_FLAGS -lib ${ANTLR_JavaScriptLexer_OUTPUT_DIR})

include_directories(${ANTLR_JavaScriptLexer_OUTPUT_DIR} ${ANTLR_JavaScriptParser_OUTPUT_DIR})

enable_testing()

add_library(parsers SHARED parsers.cpp Parser.hpp Parser.cpp Language.cpp Language.hpp ParserModule.cpp ParserModule.hpp
    Predicates.cpp Predicates.hpp
    ${ANTLR_JavaLexer_CXX_OUTPUTS} ${ANTLR_JavaParser_CXX_OUTPUTS} 
    ${ANTLR_JavaScriptLexer_CXX_OUTPUTS} ${ANTLR_JavaScriptParser_CXX_OUTPUTS} JavaScriptParserBase.cpp JavaScriptLexerBase.cpp )

#find_library(ANTLR4_LIB NAMES "libantlr4-runtime.a" HINTS "/opt/homebrew/lib")
#find_path( ANTLR_INCLUDE_DIR NAMES "antlr4-runtime.h" HINTS "/opt/homebrew/include/antlr4-runtime")

include_directories(../../include ${CMAKE_CURRENT_SOURCE_DIR})

set_target_properties(parsers PROPERTIES
    CXX_STANDARD 17
    CXX_EXTENSIONS OFF)

target_link_libraries(parsers logiccore antlr4_static)

add_test(NAME LoadModule COMMAND logical ../../../tests/parsers/loadmodule.dl)
add_test(NAME Java1 COMMAND logical java1.dl WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../../tests/parsers)
add_test(NAME Java2 COMMAND logical java2.dl WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../../tests/parsers)
add_test(NAME Js1 COMMAND logical javascript1.dl WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../../tests/parsers)
add_test(NAME Js2 COMMAND logical javascript2.dl WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../../tests/parsers)

add_custom_command(TARGET parsers POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:parsers> $<TARGET_FILE_DIR:logical>)
