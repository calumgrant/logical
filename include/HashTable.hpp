#pragma once
#include "Table.hpp"

/*
 HashTable
 */

namespace Logical
{
    namespace Internal
    {
        inline Int MakeMask() { return 0; }
        
        template<typename...Bs>
        Int MakeMask(bool b, Bs... bs)
        {
            return ((Int)b) + (MakeMask(bs...)<<1);
        }
    }

    template<bool...Binding>
    struct StaticBinding
    {
    };

    struct DynamicBinding
    {
        template<typename...Bools>
        DynamicBinding(Bools... bs) : mask(Internal::MakeMask(bs...)) {}
        
        DynamicBinding(Int m) : mask(m) {}
        
        Int mask;
    };


    namespace Internal
    {
        const int P = 317;
    
        inline Int Hash(Int i) { return i; }

        template<typename...Ints>
        Int Hash(Int i, Ints... is)
        {
            return i + P * Hash(is...);
        }

        template<bool...Binding>
        struct HashHelper;

        template<>
        struct HashHelper<>
        {
            static Int Hash() { return 0; }
            static Int Hash(const Int * row) { return 0; }
        };


        template<bool...Binding>
        struct HashHelper<true, Binding...>
        {
            template<typename...Ints>
            static Int Hash(Int i, Ints... is) { return i + P * HashHelper<Binding...>::Hash(is...); }
            static Int Hash(const Int * row) { return *row + P * HashHelper<Binding...>::Hash(row+1); }
        };

        template<bool...Binding>
        struct HashHelper<false, Binding...>
        {
            template<typename...Ints>
            static Int Hash(Int i, Ints... is) { return HashHelper<Binding...>::Hash(is...); }
            static Int Hash(const Int * row) { return HashHelper<Binding...>::Hash(row+1); }
        };
    
        template<bool...Bound, typename...Ints>
        Int BoundHash(Ints... is) { return HashHelper<Bound...>::Hash(is...); }

        template<bool...Bound>
        Int BoundHash(const Int * row) { return HashHelper<Bound...>::Hash(row); }

        inline Int Hash(DynamicBinding b, const Int * row)
        {
            Int h=0;
            Int mul = 1;
            for(auto m = b.mask; m; row++, m>>=1)
            {
                if(m&1) { h += mul * *row; mul = mul*P; }
            }
            return h;
        }
    
        template<bool...Bs>
        Int Hash(StaticBinding<Bs...> b, const Int * row)
        {
            return HashHelper<Bs...>::Hash(row);
        }

        template<bool...Bs, typename...Ints>
        Int Hash(StaticBinding<Bs...> b, Ints...is)
        {
            return HashHelper<Bs...>::Hash(is...);
        }
    
        inline Int HashWithMask(Int m) { return 0; }
    
        template<typename...Ints>
        Int HashWithMask(Int m, Int i, Ints...is)
        {
            return (m&1)? i + 317 * HashWithMask(m>>1, is...) : HashWithMask(m>>1, is...);
        }
    
        template<typename...Ints>
        Int Hash(DynamicBinding b, Int i, Ints...is)
        {
            return HashWithMask(b.mask, i, is...);
        }

        template<typename Arity>
        Int Hash(Arity arity, const Int *p)
        {
            Int h = 0;
            Int mul = 1;
            for(int i=0; i<arity.value; ++i, mul*=P)
                h += mul*p[i];
            return h;
        }
    
        inline Int Hash(DynamicArity) { return 0; }
    
        inline Int Hash(StaticArity<0>) { return 0; }
    
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
        
        template<bool...Bound, typename...Ints>
        void Find(Ints... is)
        {
            
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
}
