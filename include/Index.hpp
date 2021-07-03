#pragma once

#include "Logical.hpp"
#include "Utils.hpp"

namespace Logical
{
    enum EntityType { None };

    struct Enumerator
    {
        std::uint32_t i, j;
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
        
        template<typename Binding>
        void Find(Enumerator &e, Binding b, const Int * query)
        {
            e.i = Internal::lower_bound(arity, b, data, size, query);
            e.j = Internal::upper_bound(arity, b, data, size, query);
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
        static const int empty = 0xffffffff;
        
        HashIndex(const Int * data, const uint32_t * table, std::uint32_t size) :
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
            std::uint32_t row;
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
        const uint32_t * table;
        std::uint32_t size;
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
