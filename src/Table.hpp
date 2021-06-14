
#pragma once

/*
 Stores a table of tuples of any arity.
 */
class Table : public Receiver
{
public:
    virtual Arity GetArity() const =0;
    virtual Size Rows() const =0;
    
    // Delta management
    
    // Queries all items up to and including the current delta.
    virtual void Query(Entity * row, ColumnMask columns, Receiver & r) =0;
    
    // Queries all items in the current delta
    virtual void QueryDelta(Entity * row, ColumnMask columns, Receiver &r) =0;
    
    // Iteration management
    
    // Sets the delta to be the whole table.
    virtual void FirstIteration() =0;
    
    // Sets the
    // Adds all pending data
    virtual bool NextIteration() =0;
    
    // This is a bad interface.
    std::shared_ptr<ExecutionUnit> loop;
    
    virtual bool Add(const Entity *e) =0;
    virtual void Clear() =0;
    
    virtual void ReadAllData(Receiver&r) =0;
};
