// A test of the externs API

#include "Logical.hpp"
#include <TableWriter.hpp>
#include <cassert>
#include <sstream>
#include <iostream>
#include <vector>
#include <string>
#include <deque>

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


static void outputTable(Call & call)
{
    int args = call.ArgCount();
    auto & writer = *(TableWriter*)(call.GetData());

    if(call.First())
    {
        std::vector<std::string> row;
        row.push_back("");

        for(int i=1; i<args; ++i)
        {
            row.push_back(call.ArgName(i));
        }
        
        writer.Header(std::move(row));
        return;
    }
    
    if(call.Last())
    {
        writer.EndTable();
        std::cout << std::endl;
        return;
    }
    
    std::vector<std::string> row;
    for(int i=0; i<args; ++i)
    {
        if(call.GetMode(i) == In)
        {
            const char * v;
            Int iv;
            if(call.Get(i, v))
                row.push_back(v);
            else if(call.Get(i, iv))
            {
                std::stringstream ss;
                ss << iv;
                row.push_back(ss.str());
            }
            else
                row.push_back("?");
        }
    }
    call.CountResult();
    writer.AddRow(std::move(row));
}

void RegisterFunctions(Module & module)
{
    module.AddFunction(helloworld, {"test:helloworld"}, {Out});
    module.AddFunction(countargs, {"test:countargs"}, {Out});
    module.AddFunction(listargs, {"test:listargs", "arg1"}, {Out, Out});
    module.AddFunction(listargs, {"test:listargs"}, {Varargs});
    module.AddFunction(concat, {"test:concat"}, {Varargs});
    module.AddCommand(setdata, {"test:setdata"});

    auto tablewriter = new TableWriterImpl(std::cout, TableWriterConfig());
    module.AddFunction(outputTable, {"test:table"}, {Varargs}, tablewriter);
}
