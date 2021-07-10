// A test of the externs API

#include "Logical.hpp"

using namespace Logical;

static void helloworld(Call & call)
{
    call.Set(0, "Hello, world!");
    call.YieldResult();
}

void RegisterFunctions(Module & module)
{
    module.AddFunction(helloworld, "test:helloworld", Out);
}
