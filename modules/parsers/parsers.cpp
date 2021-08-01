#include "Parser.hpp"
#include "Language.hpp"
#include "ParserModule.hpp"

#include "JavaLexer.h"
#include "JavaParser.h"
#include "JavaScriptLexer.h"
#include "JavaScriptParser.h"
#include <memory>

Language::~Language() {}

static ParserModule parserModule;

static void parse(Logical::Call & call)
{
    const char * path;
    if(call.Get(0, path))
    {
        auto pm = (ParserModule*)call.GetData();
        pm->Parse(call, path);
    }
    else
    {
        call.Error("The supplied argument was not a string");
    }
}

static void parse_language(Logical::Call & call)
{
    const char *path, *language;
    if(call.Get(0, path) && call.Get(1, language))
    {
        auto pm = (ParserModule*)call.GetData();
        pm->Parse(call, path, language);
    }
    else
    {
        call.Error("The supplied argument was not a string");
    }
}

void RegisterFunctions(Logical::Module & module)
{
    typedef AntlrLanguage<java::JavaLexer, java::JavaParser, java::JavaParser::CompilationUnitContext, &java::JavaParser::compilationUnit> JavaParser;
    typedef AntlrLanguage<javascript::JavaScriptLexer, javascript::JavaScriptParser, javascript::JavaScriptParser::ProgramContext, &javascript::JavaScriptParser::program> JavaScriptParser;
    
    parserModule.AddLanguage(std::unique_ptr<Language>(new JavaParser("Java", ".java")));
    parserModule.AddLanguage(std::unique_ptr<Language>(new JavaScriptParser("JavaScript", ".js")));
    
    module.AddCommand(parse_language, {"parsers:parse", "language"}, &parserModule);
    module.AddCommand(parse, {"parsers:parse"}, &parserModule);
}
