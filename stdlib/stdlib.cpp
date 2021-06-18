
#include <Logical.hpp>
#include <iostream>
#include <cmath>

using namespace Logical;

static void print(Call & call)
{
    const char * str;
    if(call.Get(0, str))
        std::cout << str << std::endl;
}

static void pi(Call & call)
{
    call.Set(0, M_PI );
    call.YieldResult();
}

void RegisterFunctions(Module & module)
{
    module.RegisterFunction(print, "print", In);
    module.RegisterFunction(pi, "pi", Out);
}
