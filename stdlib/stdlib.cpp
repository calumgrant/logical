
#include <Logical.hpp>
#include <iostream>
#include <cmath>

using namespace Logical;

static void print(Call & call)
{
    const char * str;
    double d;
    long i;
    bool b;
    if(call.Get(0, str))
        std::cout << str << std::endl;
    else if(call.Get(0, d))
        std::cout << d << std::endl;
    else if(call.Get(0, i))
        std::cout << i << std::endl;
    else if(call.Get(0, b))
        std::cout << (b?"true":"false") << std::endl;
    else if(call.GetAtString(0, str))
        std::cout << "@" << str << std::endl;
    else
        std::cout << "?\n";
        
}

static void pi(Call & call)
{
    call.Set(0, M_PI);
    call.YieldResult();
}

void RegisterFunctions(Module & module)
{
    module.RegisterFunction(print, "print", In);
    module.RegisterFunction(pi, "pi", Out);
}
