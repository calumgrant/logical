
#pragma once

class Table : public Receiver
{
public:
    virtual Arity GetArity() const =0;
    virtual Size Rows() const =0;
    
    // Delta management
    // Queries all items up to and including the current delta.
    
    virtual void Query(Entity * row, ColumnMask columns, Receiver & r) =0;
    virtual void QueryDelta(Entity * row, ColumnMask columns, Receiver &r) =0;
    
    // Iteration management
    
    // Sets the delta to be the whole table.
    virtual void FirstIteration() =0;
    
    // Sets the
    // Adds all pending data
    virtual bool NextIteration() =0;
};

class EmptyTable : public Table
{
public:
};
