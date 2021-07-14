#pragma once

enum class Instruction : char
{
    Nop,
    LoadTable, // id(u8), arity(u8), table-name(cstr)
    LoadPredicate, // id(u8), arity(u8), name(cstr)
    CreateTable,  // id(u8), arity(u8)
    CreateEnumerator, // id(u8)
    CreateIndex, // tableid(u8), ...
    Return,  // table-id(u8)
    LoadString,  // variable(u8), data(cstr)
    LoadAtString, // variable(u8), data(cstr)
    LoadInt8,
    LoadInt16,
    LoadInt32,  // variable(u8), data(i32)
    LoadInt64,
    LoadDouble,
    LoadTrue,
    LoadFalse,
    LoadNone, // variable(u8)
    Add,  // table-id(u8), variable...(u8...)
    QueryIndex, NextIndex, // table-id(u8), enumerator(u8), variable...(u8...)
    QueryTable, NextTable, // Label(i16), table-id(u8), enumerator(u8), inputs...(u8), outputs...(u8)
    QueryDelta, NextDelta,
    Call, // table-id(u8), inputs...(u8), outputs...(u8)
    NextIteration, // Label(i16)
    TestAdded, // Label(i16)
    Jmp,  // label(i16)
    TestEq, // Label(i16), v1(u8), v2(u8)
    TestNeq,
    TestLt,
    TestLtEq,
    Add, // v1, v2, v3
    Sub,
    Mul,
    Div,
    Mod,
    Neg,
};

class VirtualMachine
{
public:
    void Execute(const char * function);

};
