/*
    Index.hpp
    Efficiently finding data int tables.
    As "index" is a lookup table used to locate data quickly.
    There are two main types of index: a sorted index (SortedIndex), and a hash table (HashIndex).
    A sorted index references a table of Int, stored contiguously in rows.
    A hashed index references a list of offsets into a table of rows. A "linear probing" algorithm is used,
    whereby the next item in the hash chain is at the next position in the hash table.
 
    Each index takes an Arity to store the width of the table,
    and a Binding to indicate which columns are bound (inputs) or unbound (outputs).
    There are Static- and Dynamic- versions of the arity and binding, which allows the compiler to generate
    more efficient code if the shape of the index is known at compile-time.
 
    An Enumerator is used to store the position in the index when iterating over results. The Find() method
    performs a query and sets up the Enumerator to point to the results in the index. The Next() method
    is used to retrieve a result and store the values in the supplied location.
 
    Enumerator e;
    DynamicBinding b(true, false);
    Int x=12, y;
    index.Find(e, b, x);
    while(index.Next(e,b,x,y))
    {
        std::cout << "Result is (" << x << "," << y << ")\n";
    }
 
 
 */

#pragma once
#include "Logical.hpp"
#include "Utils.hpp"

namespace Logical
{
    enum EntityType { None };

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
        bool Find(Enumerator &e, Binding)
        {
            e.i = 0;
            e.j = size;
            return size>0;
        }
        
        template<typename Binding>
        bool Next(Enumerator &e, Binding, Int * result)
        {
            if(e.i < e.j)
            {
                Internal::copy_row(arity, data+e.i, result);
                e.i += arity.value;
                return true;
            }
            return false;
        }

        template<typename Binding, typename...Ints>
        bool Next(Enumerator &e, Binding b, Ints... result)
        {
            if(e.i < e.j)
            {
                Internal::BindRow(b, data+e.i, result...);
                e.i += arity.value;
                return true;
            }
            return false;
        }
        
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
        
        template<typename Binding, typename Int>
        void Find(Enumerator &e, Binding b, Int * query)
        {
            e.i = Internal::LowerBound(arity, b, data, size, (const Int*)query);
            e.j = Internal::UpperBound(arity, b, data, size, (const Int*)query);
        }

        template<typename Binding, typename...Ints>
        void Find(Enumerator &e, Binding b, Ints... query)
        {
            e.i = Internal::LowerBound(arity, b, data, size, query...);
            e.j = Internal::UpperBound(arity, b, data, size, query...);
        }
        
        template<typename Binding>
        bool Next(Enumerator &e, Binding b, Int * result)
        {
            if(e.i < e.j)
            {
                Internal::BindRow(b, data+e.i, result);
                e.i += arity.value;
                return true;
            }
            return false;
        }

        template<typename Binding, typename...Ints>
        bool Next(Enumerator &e, Binding b, Ints&&... result)
        {
            if(e.i < e.j)
            {
                Internal::BindRow(b, data+e.i, result...);
                e.i += arity.value;
                return true;
            }
            return false;
        }

        
    private:
        Arity arity;
        const Int * data;
        std::uint32_t size;
    };

    /*
        A table where the data is stored in a hash table.
        The table contains indexes into the data, denoting the start of a row.
        Linear hashing is used so the query just needs to scan forward in the hash table
        until it reaches an empty cell storing -1.
     */
    template<typename Binding>
    class HashIndex
    {
    public:
        static const Internal::ShortIndex empty = 0xffffffff;
        
        HashIndex(const Int * data, const Internal::ShortIndex * table, Internal::ShortIndex size) :
            data(data), table(table), size(size)
        {
        }

        void Find(Enumerator &e, Binding b, const Int * query)
        {
            e.i = Internal::Hash(b, query) % size;
        };

        template<typename... Ints>
        void Find(Enumerator &e, Binding b, Ints... query)
        {
            e.i = Internal::Hash(b, query...) % size;
        };
        
        bool Next(Enumerator &e, Binding b, Int * result)
        {
            std::uint32_t row;
            while((row=table[e.i++]) != empty)
            {
                if(e.i > size) e.i -= size;
                if(Internal::BoundEquals(b, data+row, result))
                {
                    Internal::BindRow(b, data+row, result);
                    return true;
                }
            }
            return false;
        }
        
        template<typename... Ints>
        bool Next(Enumerator &e, Binding b, Ints... result)
        {
            Internal::ShortIndex row;
            while((row=table[e.i++]) != empty)
            {
                if(e.i > size) e.i -= size;
                if(Internal::BoundEquals(b, data+row, result...))
                {
                    Internal::BindRow(b, data+row, result...);
                    return true;
                }
            }
            return false;
        }
        
    private:
        const Int * data;
        const Internal::ShortIndex * table;
        Internal::ShortIndex size;
    };

    /*
        An index where the data is stored in a sorted table.
        The storage is in a "compact" 32-bit representation.
        Not implemented yet.
     */
    template<typename Arity>
    class CompactIndex
    {
    public:
    private:
        EntityType *types;
        std::int32_t * data, size;
    };    
}
