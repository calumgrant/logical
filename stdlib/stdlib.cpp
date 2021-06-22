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

void RegisterFunctions(Module & module)
{
    module.AddCommand(print, "print");
    module.AddFunction(pi, "pi", Out);
    module.AddCommand(error, "error");
    module.AddFunction(strlen, "string", In, "strlen", Out);
    module.AddFunction(lowercase, "string", In, "lowercase", Out);
    module.AddFunction(uppercase, "string", In, "uppercase", Out);
    
    module.AddCommand(expectedresults, "expected-results");
    module.AddCommand(steplimit, "evaluation-step-limit");
}
