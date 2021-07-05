#pragma once

/*
    Module to write compiled programs for Logical.
*/


#include "Logical.hpp"
#include "Utils.hpp"

/*
    void MyFunction(Call & call)
    {
        TableData p1, p2;
        JoinData join1, join2;
        Int a, b, c;
 
        InitTable(call, 0, p1);
        InitTable(call, 1, p2);

        InitJoin<2>(p1, join1);
    l1:
        if (!NextScan<2>(join1, a, b)) goto l3;
        InitJoin<2>(join2, p2, b);
    l2:
        if (!Next<2,1>(join2, c)) goto l1;
        Write(call, a, c);
        goto l2;
    l3:
    }

    void MyFunction2(Call & call)
    {
        Write(call, CreateString(call, "Hello world!"));
    }

    void RegisterFunctions(Module & module)
    {
        module.AddPredicate(MyFunction, name, dependencies);
        auto table1 = module.FindTable({ "helloworld" });
        module.AddTable(MyFunction2, "helloworld", {});
    }
*/

namespace Logical
{
}
