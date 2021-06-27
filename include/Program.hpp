#pragma once

/*
    Module to write compiled programs for Logical.
*/


#include "Logical.hpp"

/*
    void MyFunction(Call & call)
    {
        Program<4> program(call);
        Join join1, join2;
        Int a, b, c;

        join1 = program.join<0,2>();
    l1:
        if (!program.next<2,0>(join1, a, b)) goto l3;
        join2 = program.join<1,2>(b);
    l2:
        if (!program.next<1>(join2, b, c)) goto l1;
        program.write(a, c);
        goto l2;
    l3:
    }

    void MyFunction2(Call & call)
    {
        Program<1> program(call);
        program.load<0>("Hello world!");
        program.write<0>();
    }

    void RegisterFunctions(Module & module)
    {
        module.AddPredicate(MyFunction, name, dependencies);
    }
*/

namespace Logical
{
    namespace Internal
    {
        // The data stored in a computed table.
        // The data is stored deduplicated in lexographical order.
        struct TableData
        {
            const Int * data;  // Contiguous row/column data
            Int rows;  // Number of rows of data
        };
    
        class Writer
        {
        };
    }

    struct JoinData
    {
        const Int * p, *end;
    };

    // The instruction set used to run a program.
    template<int StackSize>
    class Program
    {
    public:
        Program(Call & call)
        {
            // tables = call.GetTables();
        }

        // Starts a query
        template<int Table, int Arity, int Slot, int... Slots>
        void join()
        {
            // Make arity/inputs explicit (even though it's in )
            // Layout of the scan:
            // sp[slot] = current position
            // sp[slot+1] = end position
            // sp[slot+2 .. slot+2+inputs-1] = inputs
            // sp[slot+2+inputs .. slot+2+t.arity] = outputs (copied from table)
        }

        template<int Slot, int Slot2, int...Slots>
        bool less(const Int*row) const
        {
            return sp[Slot] < *row || (sp[Slot] == *row && less<Slot2, Slots...>(row+1));
        }

        template<int Slot, int Slot2, int...Slots>
        bool greater(const Int*row) const
        {
            return sp[Slot] > *row || (sp[Slot] == *row && greater<Slot2, Slots...>(row+1));
        }

        template<int Slot>
        bool less(const Int*row) const
        {
            return sp[Slot] < *row || sp[Slot] == *row;
        }
        
        template<int Slot>
        bool greater(const Int*row) const
        {
            return sp[Slot] > *row || sp[Slot] == *row;
        }
        
        // Returns the highest pointer that is <= value
        template<int Arity, int Slot, int...Slots>
        const Int * lower_bound(const Int * p, Int n) const
        {
            Int l=0, r=n;
            while(l<r)
            {
                auto m = (l+r)>>1;
                            
                if (!less<Slot, Slots...>(p + m*Arity))
                {
                    l = m+1;
                }
                else
                {
                    r = m;
                }
            }
            return p + l*Arity;
        }
        
        template<int Arity>
        const Int * lower_bound(const Int * p, Int size) const
        {
            // Optimization of previous case
            return p;
        }

        // Returns the smallest pointer that is > value
        template<int Arity, int Slot, int...Slots>
        const Int * upper_bound(const Int * p,  Int n) const
        {
            Int l=0, r=n;
            while(l<r)
            {
                auto m = (l+r)>>1;

                if(!greater<Slot, Slots...>(p + m * Arity))
                {
                    r = m;
                }
                else
                {
                    l = m+1;
                }
            }
            return p+Arity*r;
        }
        
        template<int Arity>
        const Int * upper_bound(const Int * p, Int size) const
        {
            // Optimization of previous case
            return p + Arity * size;
        }

        // Probes a table to test if a given tuple exists in it
        template<int Table, int Arity, int... Inputs>
        bool probe()
        {
            auto & tableData = tables[Table];
            return lower_bound<Arity, Inputs...>(tableData.data, tableData.rows) < upper_bound<Arity, Inputs...>(tableData.data, tableData.rows);
        }

        // Advances the query
        template<int Table, int Arity, int Slot, int... Outputs>
        bool next()
        {
            // TODO: Skip equal values where possible.
            if(sp[Slot].i < sp[Slot+1].i)
            {
                // copy data

                //for(int i=Inputs; i<Arity; ++i)
                //    sp[Slot+i] = t.data[sp[Slot].i + i];

                // Advance the iterator
            }
            // Check iterators
            // Load values onto stack
            // Advance iterator
        }

        template<int Table, int Slot>
        void write()
        {
            static_assert(Slot>=0 && Slot<StackSize, "Slot out of range");
        }

        template<typename Slot>
        void load_int(Int value)
        {
        }

        template<int From, int To>
        void copy()
        {
            static_assert(From>=0 && From<StackSize, "From out of range");
            static_assert(To>=0 && To<StackSize, "To out of range");
            sp[To] = sp[To];
        }
        
    // protected:
        Int sp[StackSize];
        Internal::TableData * tables;
        Internal::Writer ** writers;
    };
}
