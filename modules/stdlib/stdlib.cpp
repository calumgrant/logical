#include "functions.hpp"
#include <cstring>
#include <string>

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

void RegisterFunctions(Module & module)
{
    module.AddCommand(print, {"print"});
    module.AddCommand(error, {"error"});
    module.AddCommand(expectedresults, {"expected-results"});
    module.AddCommand(steplimit, {"evaluation-step-limit"});

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
    
    // Directory listing functions
    module.AddCommand(readcsv, {"csv:read"});
}
