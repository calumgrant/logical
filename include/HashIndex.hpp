#pragma once
#include "Index.hpp"
#include "HashUtils.hpp"

namespace Logical
{
    /*
        A table where the data is stored in a hash table.
        The table contains indexes into the data, denoting the start of a row.
        Linear hashing is used so the query just needs to scan forward in the hash table
        until it reaches an empty cell storing -1.
     */
    class HashIndex
    {
    public:
        HashIndex(const Int * data, const Internal::ShortIndex * table, Internal::ShortIndex size) :
            data(data), table(table), size(size)
        {
        }

        template<typename Binding, typename... Ints>
        void Find(Enumerator &e, Binding b, Ints... query) const
        {
            Internal::FirstHash(e, Internal::BoundHash(b, query...));
        };
        
        const Int * NextRow(Enumerator &e) const
        {
            auto row = table[Internal::GetIndex(e, size)];
            if(row == Internal::EmptyCell) return nullptr;
            Internal::NextHash(e);
            return data+row;
        }
        
        template<typename Binding, typename... Ints>
        bool Next(Enumerator &e, Binding b, Ints&&... result) const
        {
            while(auto row = NextRow(e))
            {
                if(Internal::BoundEquals(b, row, result...))
                {
                    Internal::BindRow(b, row, result...);
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

    class OpenHashIndex
    {
    public:
        OpenHashIndex(const Int * data, const Internal::ShortIndex * table, const Internal::HashNode * nodes, Internal::ShortIndex size) :
            data(data), table(table), nodes(nodes), tableSize(size) {}
        
        template<typename Binding, typename... Ints>
        void Find(Enumerator &e, Binding b, Ints... query) const
        {
            auto h = Internal::BoundHash(b, query...) % tableSize;
            e.i = table[h];
        };
        
        const Int * NextRow(Enumerator &e) const
        {
            if(e.i == Internal::EmptyCell) return nullptr;
            const auto & node = nodes[e.i];
            auto result = data + node.tuple;
            e.i = node.next;
            return result;
        }

        template<typename Binding, typename... Ints>
        bool Next(Enumerator &e, Binding b, Ints&&... result) const
        {
            while(auto row = NextRow(e))
            {
                if(Internal::BoundEquals(b, row, result...))
                {
                    Internal::BindRow(b, row, result...);
                    return true;
                }
            }
            return false;
        }
        
    private:
        const Int * data;
        const Internal::ShortIndex * table;
        const Internal::HashNode * nodes;
        Internal::ShortIndex tableSize;
    };
}
