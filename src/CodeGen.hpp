#pragma once

class CodeGen
{
public:
    typedef int Variable;
    typedef int Function;
    typedef int Label;
    typedef int Table;

    Function MakeFunction();
    Variable MakeLocal();

    Table CreateTable(int arity);

};
