#pragma once
#include "Table.hpp"

/*
 HashTable
 */

namespace Logical
{
    namespace Internal
    {
        template<typename Alloc>
        class HashIndex
        {
        public:
            static const int empty = -1;
            
            HashIndex(ShortIndex len) : index(len)
            {
                items = 0;
                for(int i=0; i<len; ++i)
                    index[i] = empty;
            }
            
            int size() const { return items; }
            int capacity() const { return index.size(); }
            
            void Add(Int hash, int i)
            {
                int s = index.size();
                hash = hash % s;
                while(index[hash] != empty)
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
            
            std::vector<ShortIndex> index;
        private:
            ShortIndex items;
        };
    }


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
        
        template<typename Int>
        void Add(bool & added, Int * data)
        {
            auto h = Internal::Hash(this->arity, (const Int*)data);

            if(ProbeWithHash(h, (const Int*)data)) return;
            
            rehash();
            
            auto row = this->values.size();
            AddInternal((const Int*)data);
            index.Add(h, row);
            added = true;
        }
        
        template<typename...Ints>
        void Add(bool & added, Ints... is)
        {
            auto h = Internal::Hash(is...);

            if(ProbeWithHash(h, is...)) return;
            
            rehash();
            
            auto row = this->values.size();
            AddInternal(is...);
            index.Add(h, row);
            added = true;
        }

        template<typename Binding>
        void Find(Enumerator e, Binding b)
        {
            e.i = 0;
            e.j = this->size();
        }
        
        template<typename Binding, typename...Ints>
        void Find(Enumerator e, Binding b, Ints... is)
        {
            // Find a suitable integer???
        }
        
    private:
        
        bool ProbeWithHash(Int h, const Int * is)
        {
            int p;
            while( (p=index[h]) != HashIndex::empty)
            {
                // Compare the contents
                if(Internal::row_equals(this->arity, &this->values[p], is))
                    return true;
                
                h = h+1;
            }
            
            return false;
        }
        
        template<typename...Ints>
        bool ProbeWithHash(Int h, Ints... is)
        {
            int p;
            while( (p=index[h]) != HashIndex::empty)
            {
                // Compare the contents
                if(Internal::row_equals(&this->values[p], is...))
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

        void rehash()
        {
            if(index.capacity() < 2 * index.size())
            {
                Internal::HashIndex<Alloc> index2(2*index.capacity());
                
                for(Int i=0; i<this->values.size(); i+=this->arity.value)
                {
                    index2.Add(Internal::Hash(this->arity, &this->values[i]), i);
                }
                
                index.swap(index2);
            }
        }
    };


    /*
     To look something up in here, use the hash function on the row
     */
    template<typename Binding>
    class ExternHashTable
    {
    public:
        Binding binding;
        
        const Int **first;
        Int size;
    
        template<typename...Ints>
        Int Find(Ints... is)
        {
            return Internal::Hash(binding, is...) % size;
        }
        
        Int Find(const Int * row)
        {
            return Internal::Hash(binding, row) % size;
        }

        template<typename...Ints>
        bool Next(Int & e, Ints...is)
        {
            auto current = first[e];
            if(current && Internal::BoundEquals(binding, current, is...))
            {
                Internal::BindRow(binding, current, is...);
                e++;
                if(e>=size) e-=size;
                return true;
            }
            return false;
        }
    };
}
