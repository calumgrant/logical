/*
    Index.hpp
    Efficiently finding data in tables.
    An "index" is a lookup table used to locate data quickly.
    It is efficient for three reasons:
 
    1. The algorithms are "efficient", based on binary search or hashing
    2. The library is inlined in header files, used for native code gen.
    3. There is the option to specify the "shape" of the table (arity, indexing) at compile time to reduce run-time checks.
 
    There are two main types of index: a sorted index (SortedIndex), and a hash table (HashIndex).
    A sorted index references a table of Int (a 64-bit value), stored contiguously in rows.
    A hashed index references a list of offsets into a table of rows. A "linear probing" algorithm is used,
    whereby the next item in the hash chain is at the next position in the hash table.
 
    Each index takes an Arity to store the width of the table,
    and a Binding to indicate which columns are bound (inputs) or unbound (outputs).
    There are Static- and Dynamic- versions of the arity and binding, which allows the compiler to generate
    more efficient code if the shape of the index is known at compile-time.
 
    An Enumerator is used to store the position in the index when iterating over results. The Find() method
    performs a query and sets up the Enumerator to point to the results in the index. The Next() method
    is used to retrieve a result and store the values in the supplied location.

    Find() and Next() can take either lists of arguments, or a pointer to an array. Next() returns a bool,
    where true indicates that a result is available, or false that no more results are available.
    When Next() returns true, the values in the result are stored in the arguments (or array) passed to Next.

    The arguments to Find() and Next() are in column-order, and the arguments to Next() must be the same as the
    arguments to Find().

    Enumerator e;
    DynamicBinding bf(true, false);
    Int x=12, y;
    index.Find(e, bf, x);
    while(index.Next(e, bf, x, y))
    {
        std::cout << "Result is (" << x << "," << y << ")\n";
    }
 
 */

#pragma once
#include "Logical.hpp"
#include "Utils.hpp"

namespace Logical
{
    /*
        Data storing the state when stepping through results.
     */
    struct Enumerator
    {
        Internal::ShortIndex i, j;
    };

    /*
        An index of data stored in a table.
        There are no guarantees on the order of the data, so the only thing
        you can do is to scan it sequentially.
     */
    template<typename Arity>
    class UnsortedIndex
    {
    public:
        UnsortedIndex(Arity arity, const Int * data, std::uint32_t size) :
            arity(arity), data(data), size(size)
        {
        }

        template<typename Binding>
        void Find(Enumerator &e, Binding) const
        {
            Find(e);
        }

        void Find(Enumerator &e) const
        {
            e.i = 0;
            e.j = size;
        }
        
        const Int * NextRow(Enumerator &e) const
        {
            if(e.i < e.j)
            {
                auto row = data+e.i;
                e.i += arity.value;
                return row;
            }
            return nullptr;
        }
        
        template<typename Binding, typename...Ints>
        bool Next(Enumerator &e, Binding b, Ints&&... result) const
        {
            if(auto row = NextRow(e))
            {
                Internal::BindRow(b, row, result...);
                return true;
            }
            return false;
        }

        Internal::ShortIndex rows() const { return size; }
        
    private:
        Arity arity;
        const Int * data;
        std::uint32_t size;
    };

    /*
        A table of rows stored in lexographical order.
        This can be queried with any binding provided that first columns are bound.
        This is the default index type.
     */
    template<typename Arity>
    class SortedIndex
    {
    public:
        SortedIndex(Arity arity, const Int * data, std::uint32_t size) :
            arity(arity), data(data), size(size)
        {
        }
        
        template<typename Binding, typename...Ints>
        void Find(Enumerator &e, Binding b, Ints... query) const
        {
            e.i = Internal::LowerBound(arity, b, data, size, query...);
            e.j = Internal::UpperBound(arity, b, data, size, query...);
        }

        const Int * NextRow(Enumerator &e) const
        {
            if(e.i < e.j)
            {
                auto row = data+e.i;
                e.i += arity.value;
                return row;
            }
            return nullptr;
        }

        template<typename Binding, typename...Ints>
        bool Next(Enumerator &e, Binding b, Ints&&... result) const
        {
            if(auto row = NextRow(e))
            {
                Internal::BindRow(b, row, result...);
                return true;
            }
            return false;
        }
        
    private:
        Arity arity;
        const Int * data;
        std::uint32_t size;
    };
}
