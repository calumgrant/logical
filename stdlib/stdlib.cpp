
#include <Logical.hpp>
#include <iostream>
#include <cmath>
#include <sstream>

using namespace Logical;

static void print(Call &call, std::ostream & os)
{
    const char * str;
    double d;
    Int i;
    bool b;
    
    if(call.Get(0, str))
        os << str;
    else if(call.Get(0, d))
        os << d;
    else if(call.Get(0, i))
        os << i;
    else if(call.Get(0, b))
        os << (b?"true":"false");
    else if(call.GetAtString(0, str))
        os << "@" << str;
    else
        os << "?";
}

static void print(Call & call)
{
    print(call, std::cout);
    std::cout << std::endl;
}

static void error(Call & call)
{
    std::stringstream ss;
    print(call, ss);
    call.GetModule().ReportError(ss.str().c_str());
}

static void pi(Call & call)
{
    call.Set(0, M_PI);
    call.YieldResult();
}

static void strlen(Call & call)
{
    const char * str;
    if(call.Get(0, str))
    {
        call.Set(1, (Int)std::strlen(str));
        call.YieldResult();
    }
}

static void lowercase(Call & call)
{
    const char * str;
    if(call.Get(0, str))
    {
        auto len = std::strlen(str);
        char tmp[len+1];
        for(int i=0; i<len; ++i)
            tmp[i] = std::tolower(str[i]);
        tmp[len]=0;
        call.Set(1, tmp);
        call.YieldResult();
    }
}

static void uppercase(Call & call)
{
    const char * str;
    if(call.Get(0, str))
    {
        auto len = std::strlen(str);
        char tmp[len+1];
        for(int i=0; i<len; ++i)
            tmp[i] = std::toupper(str[i]);
        tmp[len]=0;
        call.Set(1, tmp);
        call.YieldResult();
    }
}

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
