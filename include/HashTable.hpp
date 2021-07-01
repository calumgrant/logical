#pragma once
#include "Table.hpp"

/*
 HashTable
 */

namespace Logical
{
    namespace Internal
    {
        class HashIndex
        {
        public:
            static const int empty = -1;
            
            HashIndex(int len) : index(len)
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
            
        private:
            std::vector<int> index;
            int items;
        };
    }


    template<typename Arity>
    class HashTable : public Table<Arity>
    {
    public:
        HashTable(Arity a = Arity()) : Table<Arity>(a), index(512) {}
        
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
        
        // A reentrant index.
        struct indexenumerator
        {
            HashTable<Arity> & table;
            Internal::HashIndex & index;
            int current;
            // What happens if the index resizes???
            // When is the index resized?
            // One idea is to have a shared pointer into the index.
        };
        
        template<bool...Bound>
        class Index
        {
        public:
            Index(HashTable<Arity> & table);
        };
        
        template<typename Binding, typename...Ints>
        void Find(Binding b, Ints... is)
        {
            // Find a suitable integer???
        }
        
    private:
        
        template<typename...Ints>
        bool ProbeWithHash(Int h, Ints... is)
        {
            int p;
            while( (p=index[h]) != Internal::HashIndex::empty)
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
        
        Internal::HashIndex index;

        void rehash()
        {
            if(index.capacity() < 2 * index.size())
            {
                Internal::HashIndex index2(2*index.capacity());
                
                for(Int i=0; i<this->values.size(); i+=this->arity.value)
                {
                    index2.Add(Hash(this->arity, &this->values[i]), i);
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
