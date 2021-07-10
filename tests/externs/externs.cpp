// A test of the externs API

#include "Logical.hpp"
#include <cassert>

using namespace Logical;

static void helloworld(Call & call)
{
    call.Set(0, "Hello, world!");
    call.YieldResult();
}

static void countargs(Call & call)
{
    call.Set(0, (Int)call.ArgCount());
    call.YieldResult();
}

void RegisterFunctions(Module & module)
{
    module.AddFunction(helloworld, "test:helloworld", Out);

    module.AddFunction(countargs, "test:countargs", Out);
}
