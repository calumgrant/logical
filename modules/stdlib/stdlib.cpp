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
    module.AddCommand(print, "print");
    module.AddCommand(error, "error");
    module.AddCommand(expectedresults, "expected-results");
    module.AddCommand(steplimit, "evaluation-step-limit");

    module.AddFunction(pi, "pi", Out);
    module.AddFunction(strlen, "string", In, "strlen", Out);
    module.AddFunction(lowercase, "string", In, "lowercase", Out);
    module.AddFunction(uppercase, "string", In, "uppercase", Out);
    module.AddFunction(none, "none", Out);
    
    module.AddFunction(readcontents, "file", In, "file-contents", Out);
    module.AddFunction(readlines, "file", In, "file-line", Out, "text", Out);
    module.AddCommand(writecontents, "file", "file-contents");
    module.AddCommand(writelines, "file", "file-line", "text");
    module.AddCommand(loadmodule, "load-module");
    
    module.AddFunction(regexmatch, "regex", In, "regex-match", In);
    module.AddFunction(regexmatchgroup, "regex", In, "regex-match", In, "group", Out, "value", Out);
    // module.AddFunction(regexsearch, "regex", In, "regex-search", In, "index", Out, "position", Out);
    //module.AddFunction(regexsearchgroup, "regex", In, "regex-search", In, "index", Out, "position", Out);
    
    // TODO: Add this as a table.
    module.AddFunction(messages, "std:messages", Out);
    module.AddFunction(messages, "messages", Out);
    module.AddFunction(messageoftheday, "std:message-of-the-day", Out);
    
    // Gets the nth random number.
    module.AddFunction(rand, "index", In, "random", Out);
    
    // TODO: Make a table
    module.AddFunction(environment, "key", Out, "environment", Out);
    
    // Directory listing functions
    
}
