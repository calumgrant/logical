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
    module.RegisterFunction(print, "print", In);
    module.RegisterFunction(pi, "pi", Out);
    module.RegisterFunction(error, "error", In);
    module.RegisterFunction(strlen, "string", In, "strlen", Out);
    module.RegisterFunction(lowercase, "string", In, "lowercase", Out);
    module.RegisterFunction(uppercase, "string", In, "uppercase", Out);
    
    module.RegisterFunction(expectedresults, "expected-results", In);
    module.RegisterFunction(steplimit, "evaluation-step-limit", In);
}
