cmake_minimum_required(VERSION 3.16)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

# compiler must be 11 or 14
set(CMAKE_CXX_STANDARD 17)

# required if linking to static library
add_definitions(-DANTLR4CPP_SHARED)

# using /MD flag for antlr4_runtime (for Visual C++ compilers only)
set(ANTLR4_WITH_STATIC_CRT OFF)
# add external build for antlrcpp
include(ExternalAntlr4Cpp)
# add antrl4cpp artifacts to project environment
include_directories(${ANTLR4_INCLUDE_DIRS})

set(ANTLR_EXECUTABLE ${CMAKE_CURRENT_SOURCE_DIR}/antlr-4.9.2-complete.jar)

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

add_library(parsers SHARED parsers.cpp Parser.hpp Parser.cpp Language.cpp Language.hpp ParserModule.cpp ParserModule.hpp
    Predicates.cpp Predicates.hpp
    ${ANTLR_JavaLexer_CXX_OUTPUTS} ${ANTLR_JavaParser_CXX_OUTPUTS} 
    ${ANTLR_JavaScriptLexer_CXX_OUTPUTS} ${ANTLR_JavaScriptParser_CXX_OUTPUTS}
    JavaScriptParserBase.cpp JavaScriptLexerBase.cpp )

include_directories(../../include ${CMAKE_CURRENT_SOURCE_DIR})

set_target_properties(parsers PROPERTIES
    CXX_STANDARD 17
    CXX_EXTENSIONS OFF)

target_link_libraries(parsers logiccore antlr4_shared)
