#include "functions.hpp"
#include <TableWriter.hpp>
#include <cstring>
#include <string>
#include <sstream>
#include <iostream>

static void expectedresults(Call & call)
{
    Int value;
    if(call.Get(0, value))
    {
        call.GetModule().SetExpectedResults(value);
    }
    else
    {
        call.GetModule().ReportError("Invalid number format to expected-results");
    }
}

static void expectederrors(Call & call)
{
    Int value;
    if(call.Get(0, value))
    {
        call.GetModule().SetExpectedErrors(value);
    }
    else
    {
        call.GetModule().ReportError("Invalid number format to expected-errors");
    }
}


static void steplimit(Call & call)
{
    Int value;
    if(call.Get(0, value))
    {
        call.GetModule().SetEvaluationStepLimit(value);
    }
    else
    {
        call.GetModule().ReportError("Invalid number format to evaluation-step-limit");
    }
}

extern char ** environ;

static void environment(Call & call)
{
    for(auto e = environ; *e; ++e)
    {
        auto eq = strchr(*e, '=');
        if(eq)
        {
            std::string key(*e, eq);
            call.Set(0, key.c_str());
            call.Set(1, eq+1);
            call.YieldResult();
        }
    }
}

static void loadmodule(Call &call)
{
    const char * name;
    if(call.Get(0, name))
    {
        call.GetModule().LoadModule(name);
    }
    else
    {
        call.GetModule().ReportError("Module is not a string");
    }
}

static void memorylimitmb(Call & call)
{
    Int value;
    if(call.Get(0, value))
    {
        call.GetModule().SetMemoryLimitMB(value);
    }
    else
    {
        call.GetModule().ReportError("Memory limit is not an int");
    }
}

static void import(Call & call)
{
    const char * name;
    if(call.Get(0, name))
    {
        call.Import(name);
    }
    else
    {
        call.Error("Import must be a string.");
    }
}

static void outputTable(Call & call)
{
    int args = call.ArgCount();
    auto & writer = *(TableWriter*)(call.GetData());

    if(call.First())
    {
        std::vector<std::string> row;

        for(int i=0; i<args; ++i)
        {
            row.push_back(call.ArgName(i));
        }
        
        writer.Header(std::move(row));
        return;
    }
    
    if(call.Last())
    {
        writer.EndTable();
        std::cout << std::endl;
        return;
    }
    
    std::vector<std::string> row;
    for(int i=0; i<args; ++i)
    {
        if(call.GetMode(i) == In)
        {
            const char * v;
            Int iv;
            if(call.Get(i, v))
                row.push_back(v);
            else if(call.Get(i, iv))
            {
                std::stringstream ss;
                ss << iv;
                row.push_back(ss.str());
            }
            else
                row.push_back("?");
        }
    }
    call.CountResult();
    writer.AddRow(std::move(row));
}

static void isString(Call & call)
{
    const char * str;
    if (call.Get(0, str))
        call.YieldResult();
}

static void assertString(Call & call)
{
    const char * str;
    if (call.Get(0, str))
        call.YieldResult();
    else
        call.Error("Assertion failed: string expected");
}

static void isAtString(Call & call)
{
    const char * str;
    if (call.GetAtString(0, str))
        call.YieldResult();
}

static void assertAtString(Call & call)
{
    const char * str;
    if (call.GetAtString(0, str))
        call.YieldResult();
    else
        call.Error("Assertion failed: @-string expected");
}

static void isInt(Call & call)
{
    Int i;
    if (call.Get(0, i))
        call.YieldResult();
}

static void assertInt(Call & call)
{
    Int i;
    if (call.Get(0, i))
        call.YieldResult();
    else
        call.Error("Integer expected");
}

static void isFloat(Call & call)
{
    double d;
    if (call.Get(0, d))
        call.YieldResult();
}

static void assertFloat(Call & call)
{
    double d;
    if (call.Get(0, d))
        call.YieldResult();
    else
        call.Error("Float expected");
}

static void isBool(Call & call)
{
    bool b;
    if (call.Get(0, b))
        call.YieldResult();
}

static void assertBool(Call & call)
{
    bool b;
    if (call.Get(0, b))
        call.YieldResult();
    else
        call.Error("Boolean expected");
}

static void none(Call & call)
{
    // Fail
}

static void any(Call & call)
{
    call.YieldResult();
}

static void optimizationLevel(Call & call)
{
    call.Set(0, call.GetModule().OptimizationLevel());
    call.YieldResult();
}

void RegisterFunctions(Module & module)
{
    module.AddCommand(print, {"print"});
    module.AddCommand(error, {"error"});

    module.AddFunction(errorVarargs, {"error"}, {});
    module.AddFunction(none, {"none"}, {});
    module.AddFunction(any, {"any"}, {});

    module.AddCommand(expectedresults, {"expected-results"});
    module.AddCommand(steplimit, {"evaluation-step-limit"});
    module.AddCommand(memorylimitmb, {"memory-limit-mb"});
    module.AddCommand(expectederrors, {"expected-errors"});

    module.AddFunction(pi, {"pi"}, {Out});
    module.AddFunction(strlen, {"string", "length"}, {In, Out});
    module.AddFunction(lowercase, {"string", "lowercase"}, {In, Out});
    module.AddFunction(uppercase, {"string", "uppercase"}, {In, Out});
    module.AddFunction(none, {"none"}, {Out});
    
    module.AddFunction(readcontents, {"file", "contents"}, {In, Out});
    module.AddFunction(readlines, {"file", "line", "text"}, {In, Out, Out});
    module.AddCommand(writecontents, {"file", "contents"});
    module.AddCommand(writelines, {"file", "line", "text"});
    module.AddCommand(loadmodule, {"load-module"});
    module.AddCommand(import, {"import"});
    
    module.AddFunction(regexmatch, {"regex", "match"}, {In, In});
    module.AddFunction(regexmatchgroup, {"regex", "match", "group", "value"}, {In, In, Out, Out});
    // module.AddFunction(regexsearch, "regex", In, "regex-search", In, "index", Out, "position", Out);
    //module.AddFunction(regexsearchgroup, "regex", In, "regex-search", In, "index", Out, "position", Out);
    
    // TODO: Add this as a table.
    module.AddFunction(messages, {"std:messages"}, {Out});
    module.AddFunction(messages, {"messages"}, {Out});
    module.AddFunction(messageoftheday, {"std:message-of-the-day"}, {Out});
    
    // Gets the nth random number.
    module.AddFunction(rand, {"random", "value"}, {In, Out});
    
    // TODO: Make a table
    module.AddFunction(environment, {"environment", "value"}, {Out, Out});
    
    // TODO: Directory listing functions
    module.AddCommand(readcsv, {"csv:read"});
    
    auto tablewriter = new TableWriterImpl(std::cout, TableWriterConfig());
    module.AddFunction(outputTable, {"std:query"}, { Varargs }, tablewriter);
    module.AddFunction(outputTable, {"query"}, { Varargs }, tablewriter);
    module.AddFunction(outputTable, {"select"}, { Varargs }, tablewriter);

    // Type tests
    module.AddFunction(isString, {"is-string"}, {In});
    module.AddFunction(assertString, {"assert-string"}, {In});
    module.AddCommand(assertString, {"assert-string"});
    
    module.AddFunction(isAtString, {"is-at-string"}, {In});
    module.AddFunction(assertAtString, {"assert-at-string"}, {In});
    module.AddCommand(assertAtString, {"assert-at-string"});

    module.AddFunction(isInt, {"is-int"}, {In});
    module.AddFunction(assertInt, {"assert-int"}, {In});
    module.AddCommand(assertInt, {"assert-int"});
    
    module.AddFunction(isFloat, {"is-float"}, {In});
    module.AddFunction(assertFloat, {"assert-float"}, {In});
    module.AddCommand(assertFloat, {"assert-float"});

    module.AddFunction(isBool, {"is-bool"}, {In});
    module.AddFunction(assertBool, {"assert-bool"}, {In});
    module.AddCommand(assertBool, {"assert-bool"});

    module.AddFunction(optimizationLevel, {"optimization-level"}, {Out});

    // String character functions
    module.AddFunction(stringcharacterpositionBFF, {"string", "character", "position"}, {In,Out,Out});
    module.AddFunction(stringcharacterpositionBBF, {"string", "character", "position"}, {In,In,Out});
    module.AddFunction(stringcharacterpositionBFB, {"string", "character", "position"}, {In,Out,In});
    module.AddFunction(stringcharacterpositionBBB, {"string", "character", "position"}, {In,In,In});

    module.AddFunction(stringtail, {"string","tail"}, {In,Out});
    module.AddFunction(stringhead, {"string","head"}, {In,Out});
}
