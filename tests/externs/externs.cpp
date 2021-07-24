// A test of the externs API

#include "Logical.hpp"
#include <cassert>
#include <sstream>

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

static void listargs(Call & call)
{
    int args = call.ArgCount();
    for(int i=0; i<args; ++i)
    {
        if(call.GetMode(i) == Out)
        {
            call.Set(i, call.ArgName(i));
        }
        else
        {
            const char * arg;
            call.Get(i, arg);
            if(strcmp(arg, call.ArgName(i))!=0)
                return;
        }
    }
    call.YieldResult();
}

static void concat(Call & call)
{
    std::ostringstream ss;
    int args = call.ArgCount();
    for(int i=1; i<args; ++i)
    {
        if(i>1) ss << " ";
        ss << call.ArgName(i) << "=";
        
        if(call.GetMode(i) == In)
        {
            const char * v;
            if(call.Get(i, v))
                ss << v;
            else ss << "?";
        }
        else
            ss << "_";
    }
    call.Set(0, ss.str().c_str());
    call.YieldResult();
}

static void setdata(Call & call)
{
    Call & getdata = call.GetModule().GetPredicate({"test:getdata"});

    Int data;
    if(call.Get(0, data))
    {
        getdata.Set(0, data);
        getdata.YieldResult();
    }
}

void RegisterFunctions(Module & module)
{
    module.AddFunction(helloworld, {"test:helloworld"}, {Out});
    module.AddFunction(countargs, {"test:countargs"}, {Out});
    module.AddFunction(listargs, {"test:listargs", "arg1"}, {Out, Out});
    module.AddFunction(listargs, {"test:listargs"}, {Varargs});
    module.AddFunction(concat, {"test:concat"}, {Varargs});
    module.AddCommand(setdata, {"test:setdata"});
}
