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

static void none(Call & call)
{
    // Fails always
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
        row.push_back("");

        for(int i=1; i<args; ++i)
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


void RegisterFunctions(Module & module)
{
    module.AddCommand(print, {"print"});
    module.AddCommand(error, {"error"});
    module.AddCommand(expectedresults, {"expected-results"});
    module.AddCommand(steplimit, {"evaluation-step-limit"});
    module.AddCommand(memorylimitmb, {"memory-limit-mb"});

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
}
