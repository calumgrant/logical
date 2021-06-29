#pragma once
#include "Table.hpp"

/*
 HashTable
 */

namespace Logical
{
    inline Int Hash(Int i) { return i; }

    template<typename...Ints>
    Int Hash(Int i, Ints... is)
    {
        return i + 317 * Hash(is...);
    }

    template<typename Arity>
    Int Hash(Arity arity, const Int *p)
    {
        Int h = p[arity.value-1];
        for(int i=arity.value-2; i>=0; --i)
            h = h*317 + p[i];
        return h;
    }

    class HashIndex
    {
    public:
        HashIndex(int len) : index(len)
        {
            items = 0;
            for(int i=0; i<len; ++i)
                index[i] = -1;
        }
        
        int size() const { return items; }
        int capacity() const { return index.size(); }
        
        void Add(Int hash, int i)
        {
            int s = index.size();
            hash = hash % s;
            while(index[hash] != -1)
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
        }
        
    private:
        std::vector<int> index;
        int items;
    };

    template<typename Arity>
    class HashTable : public Table<Arity>
    {
    public:
        HashTable(Arity a) : Table<Arity>(a), index(512) {}
        
        template<typename...Ints>
        bool Add(Ints... is)
        {
            auto h = Hash(is...);

            if(ProbeWithHash(h, is...)) return false;
            
            rehash();
            
            auto row = this->values.size();
            AddInternal(is...);
            index.Add(h, row);

            return true;
        }
                
        template<typename...Ints>
        bool Probe(Ints... is)
        {
            return ProbeWithHash(Hash(is...), is...);
        }
        
        // A reentrant enumerator
        struct tableenumerator
        {
            HashTable<Arity> & table;
            int current;
        };
        
        struct indexenumerator
        {
            HashTable<Arity> & table;
            HashIndex & index;
            int current;
        };
        
    private:
        
        template<typename...Ints>
        bool ProbeWithHash(Int h, Ints... is)
        {
            int p;
            while( (p=index[h]) != -1)
            {
                // Compare the contents
                if(row_equals(&this->values[p], is...))
                    return true;
                
                h = h+1;
            }
            
            return false;
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
        
        HashIndex index;

        void rehash()
        {
            if(index.capacity() < 2 * index.size())
            {
                HashIndex index2(2*index.capacity());
                
                for(Int i=0; i<this->values.size(); i+=this->arity.value)
                {
                    index2.Add(Hash(this->arity, &this->values[i]), i);
                }
                
                index.swap(index2);
            }
        }
    };


    template<typename Arity>
    class QueryableHashTable : public HashTable<Arity>
    {
        class iterator
        {

        };

        template<typename...Ints>
        iterator Find(Ints...is);
    };
}
