#pragma once

/*
    Module to write compiled programs for Logical.
*/


#include "Logical.hpp"

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
    // The data stored in a computed table.
    // The data is stored deduplicated in lexographical order.
    struct TableData
    {
        const Int * data;  // Contiguous row/column data
        Int rows;  // Number of rows of data
    };

    namespace Internal
    {
    
        class Writer
        {
        };

        inline bool less(const Int*row, Int v);
        inline bool greater(const Int*row, Int v);

        bool less(const Int*row, Int v, Int vs...)
        {
            return v < *row || (v == *row && less(row+1, vs));
        }

        bool greater(const Int*row, Int v, Int vs...)
        {
            return v > *row || (v == *row && greater(row+1, vs));
        }

        inline bool less(const Int*row, Int v)
        {
            return v < *row;
        }
        
        bool greater(const Int*row, Int v)
        {
            return v > *row;
        }
        
        // Returns the highest pointer that is <= value
        template<int Arity, typename ... Ints>
        const Int * lower_bound(const Int * p, Int n, Ints ... vs)
        {
            Int l=0, r=n;
            while(l<r)
            {
                auto m = (l+r)>>1;
                            
                if (greater(p + m*Arity, vs...))
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
        const Int * lower_bound(const Int * p, Int n)
        {
            // Optimization of previous case
            return p;
        }

        // Returns the smallest pointer that is > value
        template<int Arity, typename...Ints>
        const Int * upper_bound(const Int * p, Int n, Ints... vs)
        {
            Int l=0, r=n;
            while(l<r)
            {
                auto m = (l+r)>>1;

                if(less(p + m * Arity, vs...))
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
        const Int * upper_bound(const Int * p, Int size)
        {
            // Optimization of previous case
            return p + Arity * size;
        }

        void copy_row(const Int *p) {}
        void copy_row(const Int *p, Int &v) { v = *p; }
    
        template<typename...Ints>
        void copy_row(const Int *p, Int &v, Ints&...vs) { v = *p; copy_row(p+1, vs...); }
    
        bool equals_row(const Int *p) { return true; }
        bool equals_row(const Int *p, Int v) { return v == *p; }
    
        template<typename...Ints>
        bool equals_row(const Int *p, Int v, Ints... vs) { return v == *p && equals_row(p+1, vs...); }

                                                           
    }

    struct JoinData
    {
        const Int * p, *end;
    };

    bool Empty(const JoinData &join)
    {
        return join.p == join.end;
    }

    bool Next(JoinData &join)
    {
        if(join.p<join.end)
        {
            join.p = join.end;
            return true;
        }
        return false;
    }

    template<int Arity>
    Int RawCount(JoinData & join)
    {
        return (join.end-join.p)/Arity;
    }

    template<int Arity, int BoundCols, typename...Ints>
    bool Next(JoinData & join, Ints & ... vs)
    {
        if(join.p<join.end)
        {
            Internal::copy_row(join.p+BoundCols, vs...);
            do
            {
                // Skip over equal results
                join.p += Arity;
            } while(join.p<join.end && Internal::equals_row(join.p+BoundCols, vs...));
            return true;
        }
        return false;
    }

    template<int Arity, typename...Ints>
    bool NextScan(JoinData & join, Ints & ... vs)
    {
        if(join.p<join.end)
        {
            Internal::copy_row(join.p, vs...);
            join.p += Arity;
            // No need to deduplicate
            return true;
        }
        return false;
    }
 
    template<int Arity, typename...Ints>
    void InitJoin(TableData & table, JoinData & join, Ints... vs)
    {
        join.p = Internal::lower_bound<Arity>(table.data, table.rows, vs...);
        join.end = Internal::upper_bound<Arity>(table.data, table.rows, vs...);
    }

    template<int Arity>
    void InitScan(TableData & table, JoinData & join)
    {
        join.p = Internal::lower_bound<Arity>(table.data, table.rows);
        join.end = Internal::upper_bound<Arity>(table.data, table.rows);
    }
        
    bool Probe(TableData & table)
    {
        return table.rows>0;
    }
    
    template<int Arity, typename... Ints>
    bool Probe(TableData & table, Ints... vs)
    {
        JoinData join;
        InitJoin<Arity>(table, join, vs...);
        return !Empty(join);
    }
}
