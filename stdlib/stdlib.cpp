#include "functions.hpp"

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
    
    module.AddFunction(readcontents, "file", In, "contents", Out);
    module.AddFunction(readlines, "file", In, "line", Out, "text", Out);
    module.AddCommand(writecontents, "file", "contents");
    module.AddCommand(writelines, "file", "line", "text");
    
    module.AddFunction(regexmatch, "regex", In, "regex-match", In);
    module.AddFunction(regexmatchgroup, "regex", In, "regex-match", In, "group", Out, "value", Out);
    // module.AddFunction(regexsearch, "regex", In, "regex-search", In, "index", Out, "position", Out);
    //module.AddFunction(regexsearchgroup, "regex", In, "regex-search", In, "index", Out, "position", Out);

}
