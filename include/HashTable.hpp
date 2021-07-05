#pragma once
#include "Table.hpp"

/*
 HashTable
 */

namespace Logical
{
    namespace Internal
    {
        /*
            A table used to store a hash map, for example for ALL columns
            in a table, or combinations of them.
            A linear probing algorithm is used on hash collision.
            Each cell stores the index of the row in the table, or EmptyCell (-1) to indicate
            that the cell is empty.
         */
        template<typename Alloc>
        class HashIndex
        {
        public:
            HashIndex(ShortIndex len) : index(len, EmptyCell)
            {
            }
            
            int size() const { return items; }
            int capacity() const { return index.size(); }
            
            void Add(Int hash, int i)
            {
                int s = index.size();
                hash = hash % s;
                while(index[hash] != EmptyCell)
                {
                    hash++;
                    if(hash>=s) hash-=s;
                }
                index[hash] = i;
                ++items;
            }

            int operator[](int i) const { return index[i%index.size()]; }
            
            void swap(HashIndex & other)
            {
                index.swap(other.index);
                std::swap(items, other.items);
            }
            
            std::vector<ShortIndex> index;
        private:
            ShortIndex items = 0;
        };
    }

    /*
        An index over a set of columns.
        There is no need to index the entire table as this is already indexed.
     */
    template<typename Arity, typename Alloc, typename Binding>
    class HashColumns
    {
    public:
        HashColumns(const Table<Arity, Alloc> & source, Binding binding) :
            source(source), binding(binding), index(128) {}

        // Adding data to the underlying table invalidates this index
        // Calling NextIteration invalidates this index
        HashIndex GetIndex()
        {
            NextIteration();
            return HashIndex(source.values.data(), index.index.data(), index.index.size());
        }

        void NextIteration()
        {
            if(source.size() * 2 > index.capacity())
            {
                // Trigger a rehash
                index = Internal::HashIndex<Alloc>(2 * source.size());
            }
            
            for(auto row = index.size(); row<source.size(); ++row)
            {
                auto i = row * source.arity.value;
                auto h = Internal::P * Internal::BoundHash(binding, &source.values[i]);
                index.Add(h, i);
            }
        }

        // Calling NextIteration() invalidates this enumerator
        template<typename... Ints>
        void Find(Enumerator &e, Binding b, Ints... query) const
        {
            e.i = (Internal::P * Internal::BoundHash(b, query...)) % index.capacity();
        };
        
        const Int * NextRow(Enumerator &e) const
        {
            auto row = index.index[e.i++];
            if(e.i >= index.capacity()) e.i -= index.capacity();
            return row==Internal::EmptyCell ? nullptr : source.values.data()+row;
        }
        
        template<typename... Ints>
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
        const Table<Arity> & source;
        Binding binding;
        Internal::HashIndex<Alloc> index;
    };


    /*
     A hash table that supports basic lookups.
     */
    template<typename Arity, typename Alloc = std::allocator<Int>>
    class BasicHashTable : public Table<Arity, Alloc>
    {
    public:
        BasicHashTable(Arity a = Arity()) : Table<Arity>(a), index(256) {}
        
        typedef UnsortedIndex<Arity> ScanIndexType;
        typedef HashIndex ProbeIndexType;
        
        // Note that calling Add() invalidates this index.
        ScanIndexType GetScanIndex() const
        {
            return ScanIndexType(this->arity, this->values.data(), this->values.size()/this->arity.value);
        }
        
        ProbeIndexType GetProbeIndex()
        {
            return HashIndex(this->values.data(), index.index.data(), index.capacity());
        }
        
        template<typename...Ints>
        void Add(bool & added, Ints... is)
        {
            auto h = Internal::Hash(this->arity, is...) * Internal::P;

            if(ProbeWithHash(h, is...)) return;
            
            Rehash();
            
            auto row = this->values.size();
            AddInternal(is...);
            index.Add(h, row);
            added = true;
        }
        
        /*
            Enumerates the contents of the hashtable.
            Enumerates
         */
        const Int * NextRow(Enumerator &e) const
        {
            if(e.i < e.j)
            {
                auto row = &this->values[e.i];
                e.i += this->arity.value;
                return row;
            }
            return nullptr;
        }
        
        template<typename Binding, typename...Ints>
        bool Next(Enumerator &e, Binding b, Ints&&...values) const
        {
            if(auto p = NextRow(e))
            {
                Internal::BindRow(b, p, values...);
                return true;
            }
            return false;
        }

        
        template<typename Binding>
        HashColumns<Arity, Alloc, Binding> MakeIndex(Binding binding) const
        {
            return HashColumns<Arity, Alloc, Binding>(*this, binding);
        }
        
    private:

        template<typename...Ints>
        bool ProbeWithHash(Int h, Ints... is)
        {
            int p;
            while( (p=index[h]) != Internal::EmptyCell)
            {
                // Compare the contents
                if(Internal::row_equals(this->arity, &this->values[p], is...))
                    return true;
                
                h = h+1;
            }
            
            return false;
        }
        
        void AddInternal(const Int * p)
        {
            for(int i=0; i<this->arity.value; ++i)
                AddInternal(p[i]);
        }

        void AddInternal(Int i)
        {
            this->values.push_back(i);
        }

        template<typename...Ints>
        void AddInternal(Int i, Ints... is)
        {
            this->values.push_back(i);
            AddInternal(is...);
        }
        
        Internal::HashIndex<Alloc> index;

        void Rehash()
        {
            if(index.capacity() < 2 * index.size())
            {
                Internal::HashIndex<Alloc> index2(2*index.capacity());
                
                for(Int i=0; i<this->values.size(); i+=this->arity.value)
                {
                    index2.Add(Internal::P * Internal::Hash(this->arity, &this->values[i]), i);
                }
                
                index.swap(index2);
            }
        }
    };

    /*
        A hash table that supports "deltas" and iterations.
        Call NextIteration() to move to the next iteration.
     */
    template<typename Arity, typename Alloc = std::allocator<Int>>
    class HashTable : public BasicHashTable<Arity, Alloc>
    {
    public:
        HashTable(Arity a) : BasicHashTable<Arity, Alloc>(a) { }

        void NextIteration()
        {
            deltaStart = deltaEnd;
            deltaEnd = this->values.size();
        }

        /*
            Queries the whole table up to and including the previous iterations.
            Items added by the current iteration are not returned.
         */
        template<typename Binding>
        void Find(Enumerator &e, Binding) const
        {
            e.i = 0;
            e.j = deltaEnd;
        }

        /*
            Queries items only in the delta.
         */
        template<typename Binding>
        void FindDelta(Enumerator &e, Binding) const
        {
            e.i = deltaStart;
            e.j = deltaEnd;
        }
                
    private:
        Internal::ShortIndex deltaStart = 0, deltaEnd = 0;
    };
}
