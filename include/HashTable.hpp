#pragma once
#include "Table.hpp"
#include "HashIndex.hpp"

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
            HashIndex(ShortIndex len, Alloc alloc=Alloc()) : index(len, EmptyCell, alloc)
            {
            }

            template<typename Alloc2>
            HashIndex(const HashIndex<Alloc2> & src, Alloc alloc=Alloc()) : index(src.index.begin(), src.index.end(), alloc)
            {
            }

            
            int size() const { return items; }
            int capacity() const { return index.size(); }
            
            void Add(Enumerator &e, ShortIndex i)
            {
                ShortIndex s = index.size();
                while(index[GetIndex(e, s)] != EmptyCell)
                {
                    NextHash(e);
                }
                index[GetIndex(e,s)] = i;
                ++items;
            }
            
            void AddAt(const Enumerator &e, ShortIndex i)
            {
                index[GetIndex(e, index.size())] = i;
                ++items;
            }

            ShortIndex operator[](const Enumerator &e) const { return index[GetIndex(e, index.size())]; }
            
            void swap(HashIndex & other)
            {
                index.swap(other.index);
                std::swap(items, other.items);
            }
            
            std::vector<ShortIndex, typename std::allocator_traits<Alloc>::template rebind_alloc<ShortIndex>> index;

            HashIndex & operator=(HashIndex&&src)
            {
                swap(src);
                return *this;
            }
            
            HashIndex(HashIndex && src) : index(std::move(src.index)), items(std::move(src.items))
            {
            }

            HashIndex(const HashIndex & src) : index(src.index), items(src.items)
            {
            }

        private:
            ShortIndex items = 0;
        };
    }

    /*
        An index over a set of columns.
        There is no need to index the entire table as this is already indexed.
     */
    template<typename Arity, typename Binding, typename Alloc=std::allocator<Int> >
    class HashColumns
    {
    public:
        HashColumns(const Table<Arity, Alloc> & source, Binding binding) :
            source(source), binding(binding), indexSize(0), index(Internal::GetIndexSize(0), source.values.get_allocator())
        {
            NextIteration();
        }

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
                Internal::ShortIndex newsize;
                do
                {
                    newsize = Internal::GetIndexSize(++indexSize);
                }
                while(newsize < 2 * source.size());

                index = Internal::HashIndex<Alloc>(newsize, source.values.get_allocator());
            }
            
            for(auto row = index.size(); row<source.size(); ++row)
            {
                auto i = row * source.arity.value;
                Enumerator e;
                Internal::FirstHash(e, Internal::BoundHash(binding, &source.values[i]));
                index.Add(e, i);
            }
        }

        // Calling NextIteration() invalidates this enumerator
        template<typename... Ints>
        void Find(Enumerator &e, Binding b, Ints... query) const
        {
            Internal::FirstHash(e, Internal::BoundHash(b, query...));
        };
        
        const Int * NextRow(Enumerator &e) const
        {
            auto row = index[e];
            if(row == Internal::EmptyCell) return nullptr;
            Internal::NextHash(e);
            return source.values.data()+row;
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
        const Table<Arity, Alloc> & source;
        Binding binding;
        Internal::HashIndex<Alloc> index;
        int indexSize = 0;
    };


    /*
     A hash table that supports basic lookups.
     */
    template<typename Arity, typename Alloc = std::allocator<Int>>
    class BasicHashTable : public Table<Arity, Alloc>
    {
    public:
        BasicHashTable(Arity a = Arity(), Alloc alloc = Alloc()) :
            Table<Arity, Alloc>(a, alloc), indexSize(0), index(Internal::GetIndexSize(indexSize), alloc) {}
        
        template<typename Alloc2>
        BasicHashTable(const BasicHashTable<Arity, Alloc2> & src, Alloc alloc = Alloc()) :
            Table<Arity, Alloc>(src, alloc), indexSize(src.indexSize), index(src.index, alloc) {}
        
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
            if(Add(is...))
                added = true;
        }
        
        template<typename...Ints>
        bool Add(Ints... is)
        {
            Rehash();
            
            Enumerator e;

            if(Probe(e, is...))
                return false;
            
            auto row = this->values.size();
            index.AddAt(e, this->values.size());
            AddInternal(is...);
            return true;
        }
        
        /*
            Enumerates the contents of the hashtable.
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
        HashColumns<Arity, Binding, Alloc> MakeIndex(Binding binding) const
        {
            return HashColumns<Arity, Binding, Alloc>(*this, binding);
        }

        int indexSize;
        Internal::HashIndex<Alloc> index;

    private:

        // Returns the cell where the row lives, OR
        // an empty cell where we can insert the data.
        template<typename...Ints>
        bool Probe(Enumerator &e, Ints... is)
        {
            Internal::FirstHash(e, Internal::Hash(this->arity, is...));

            Internal::ShortIndex p;
            while( (p=index[e]) != Internal::EmptyCell)
            {
                // Compare the contents
                if(Internal::row_equals(this->arity, &this->values[p], is...))
                    return true;
                
                Internal::NextHash(e);
            }
            
            return false; // Not found
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
        
        void Rehash()
        {
            if(index.capacity() < 2 * index.size())
            {
                Internal::HashIndex<Alloc> index2(Internal::GetIndexSize(++indexSize), this->values.get_allocator());
                
                for(Int i=0; i<this->values.size(); i+=this->arity.value)
                {
                    Enumerator e;
                    Internal::FirstHash(e, Internal::Hash(this->arity, &this->values[i]));
                    index2.Add(e, i);
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
        HashTable(Arity a, Alloc alloc = Alloc()) : BasicHashTable<Arity, Alloc>(a, alloc) { }
        
        template<typename Alloc2>
        HashTable(const HashTable<Arity, Alloc2> & src, Alloc alloc = Alloc()) : BasicHashTable<Arity, Alloc>(src, alloc)
        {
        }

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
            Find(e);
        }
        
        void Find(Enumerator &e) const
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
            FindDelta(e);
        }
        
        void FindDelta(Enumerator &e) const
        {
            e.i = deltaStart;
            e.j = deltaEnd;
        }
        
        void clear() { this->values.clear(); deltaStart = deltaEnd = 0; }

    private:
        Internal::ShortIndex deltaStart = 0, deltaEnd = 0;
    };

    namespace Internal
    {
        static const int LoadFactor = 2;
        static const int InitialSize = 128;

        template<typename Alloc = std::allocator<Int>>
        class OpenHashIndex
        {
        public:
            OpenHashIndex(ShortIndex size, Alloc a) : table(size<InitialSize ? InitialSize : size, EmptyCell, a), nodes(a) {}

            template<typename Alloc2>
            OpenHashIndex(const OpenHashIndex<Alloc2> & src, Alloc a) :
                table(src.table.begin(), src.table.end(), a), nodes(src.nodes.begin(), src.nodes.end(), a) {}

            void Add(ShortIndex tuple, ShortIndex hash)
            {
                auto cell = hash % table.size();
                auto node = nodes.size();
                nodes.push_back( HashNode { tuple, table[cell] });
                table[cell] = node;
            }
            
            void First(ShortIndex & p, ShortIndex hash)
            {
                p = table[hash % table.size()];
            }
            
            ShortIndex Next(ShortIndex & p)
            {
                if(p == EmptyCell) return EmptyCell;
                auto result = nodes[p].tuple;
                p = nodes[p].next;
                return result;
            }
            
            // The number of items stored in the table.
            ShortIndex size() const { return nodes.size(); }
            
            void swap(OpenHashIndex & other)
            {
                table.swap(other.table);
                nodes.swap(other.nodes);
            }
            
            std::vector<ShortIndex, typename std::allocator_traits<Alloc>::template rebind_alloc<ShortIndex>> table;
            std::vector<HashNode, typename std::allocator_traits<Alloc>::template rebind_alloc<HashNode>> nodes;
        };
    }

    /*
        An index over a set of columns.
        There is no need to index the entire table as this is already indexed.
     */
    template<typename Arity, typename Binding, typename Alloc=std::allocator<Int> >
    class OpenHashColumns
    {
    public:
        OpenHashColumns(const Table<Arity, Alloc> & source, Binding binding) :
            source(source), binding(binding), index(source.values.size(), source.values.get_allocator())
        {
            NextIteration();
        }
        
        void NextIteration()
        {
            if(index.table.size() < Internal::LoadFactor * source.size())
            {
                // Create an empty index and repopulate it
                Internal::OpenHashIndex<Alloc> index2(source.size(), index.table.get_allocator());
                index.swap(index2);
            }
            
            while(index.size() < source.size())
            {
                int row = index.size() * source.arity.value;
                auto h = Internal::BoundHash(binding, &source.values[row]);
                index.Add(row, h);
            }
        }
        
        OpenHashIndex GetIndex()
        {
            NextIteration();
            return OpenHashIndex(source.values.data(), index.table.data(), index.nodes.data(), index.table.size());
        }
        
        // Calling NextIteration() invalidates this enumerator
        template<typename... Ints>
        void Find(Enumerator &e, Binding b, Ints... query) const
        {
            auto h = Internal::BoundHash(b, query...) % index.table.size();
            e.i = index.table[h];
        };
        
        const Int * NextRow(Enumerator &e) const
        {
            if(e.i == Internal::EmptyCell) return nullptr;
            const auto & node = index.nodes[e.i];
            auto result = source.values.data() + node.tuple;
            e.i = node.next;
            return result;
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
        const Table<Arity, Alloc> & source;
        Binding binding;
        Internal::OpenHashIndex<Alloc> index;
    };

    template<typename Arity, typename Alloc = std::allocator<Int>>
    class OpenHashTable : public Table<Arity, Alloc>
    {
    public:
        OpenHashTable(Arity a, Alloc alloc = Alloc()) : Table<Arity, Alloc>(a, alloc), index(Internal::InitialSize, alloc) { }

        template<typename Alloc2>
        OpenHashTable(const OpenHashTable<Arity, Alloc2> & src, Alloc alloc) : Table<Arity, Alloc>(src, alloc), index(src.index, alloc) {}
        
        template<typename... Values>
        void Add(bool & added, Values... values)
        {
            if(Add(values...)) added = true;
        }
        
        template<typename... Values>
        bool Add(Values... values)
        {
            auto h = Internal::Hash(this->arity, values...) % index.table.size();
            
            Internal::ShortIndex i = index.table[h];
            
            for(; i!=Internal::EmptyCell; i = index.nodes[i].next)
            {
                if(Internal::row_equals(this->arity, &this->values[index.nodes[i].tuple], values...))
                    return false;
            }
            
            // Maybe rehash (todo)
            if(index.nodes.size() > Internal::LoadFactor * index.table.size())
            {
                Internal::OpenHashIndex<Alloc> index2(index.nodes.size(), index.nodes.get_allocator());

                for(Internal::ShortIndex s=0; s<this->values.size(); s+=this->arity.value)
                {
                    auto h2 = Internal::Hash(this->arity, &this->values[s]) % index2.table.size();
                    index2.Add(s, h2);
                }
                index.swap(index2);
                h = Internal::Hash(this->arity, values...) % index.table.size();
            }
            
            auto s = this->values.size();
            AddInternal(values...);
            
            index.Add(s, h);
        
            return true;
        }

        typedef UnsortedIndex<Arity> ScanIndexType;
        typedef OpenHashIndex ProbeIndexType;
        
        // Note that calling Add() invalidates this index.
        ScanIndexType GetScanIndex() const
        {
            return ScanIndexType(this->arity, this->values.data(), this->values.size()/this->arity.value);
        }
        
        ProbeIndexType GetProbeIndex() const
        {
            return ProbeIndexType(this->values.data(), index.table.data(), index.nodes.data(), index.table.size());
        }
        
        template<typename Binding>
        OpenHashColumns<Arity, Binding, Alloc> MakeIndex(Binding b)
        {
            return OpenHashColumns<Arity, Binding, Alloc>(*this, b);
        }
        
        /*
            Enumerates the contents of the hashtable.
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
        
        Internal::OpenHashIndex<Alloc> index;
        
    private:
        
        void AddInternal(const Int *e)
        {
            for(int i=0; i<this->arity.value; ++i)
                AddInternal(e[i]);
        }

        void AddInternal()
        {}
        
        template<typename...Values>
        void AddInternal(Int value, Values...values)
        {
            this->values.push_back(value);
            AddInternal(values...);
        }
    };

    /*
        A hash table that supports "deltas" and iterations.
        Call NextIteration() to move to the next iteration.
     */
    template<typename Arity, typename Alloc = std::allocator<Int>>
    class DeltaHashTable : public OpenHashTable<Arity, Alloc>
    {
    public:
        DeltaHashTable(Arity a, Alloc alloc = Alloc()) : OpenHashTable<Arity, Alloc>(a, alloc) { }
        
        template<typename Alloc2>
        DeltaHashTable(const DeltaHashTable<Arity, Alloc2> & src, Alloc alloc = Alloc()) : OpenHashTable<Arity, Alloc>(src, alloc)
        {
        }

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
            Find(e);
        }
        
        void Find(Enumerator &e) const
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
            FindDelta(e);
        }
        
        void FindDelta(Enumerator &e) const
        {
            e.i = deltaStart;
            e.j = deltaEnd;
        }
        
        void clear() { this->values.clear(); deltaStart = deltaEnd = 0; }

    private:
        Internal::ShortIndex deltaStart = 0, deltaEnd = 0;
    };


    namespace Experimental
    {
        inline Int MakeTuple(Internal::ShortIndex head, Internal::ShortIndex tail)
        {
            return (Int(head)<<32) | tail;
        }
    
        inline Internal::ShortIndex Head(Int i) { return i>>32; }
        inline Internal::ShortIndex Tail(Int i) { return (Internal::ShortIndex)i; }


        template<typename Alloc>
        class TupleStore
        {
        public:
            TupleStore(Alloc a) : values(a) {}
            
            //
            // value is opaque - it stores either an entity (in a 1-tuple), or
            // a pair of indexes - head and a tail, constructing a tuple.
            Internal::ShortIndex Find(Int value)
            {
                // Look up existing value or create a new one.
                Internal::ShortIndex result;
                auto hash = value ^ (value>>32);
                
                index.Find(result);
                if(result == Internal::EmptyCell)
                {
                    result = values.size();
                    values.push_back(value);
                    index.Add(result);
                }
                return result;
            }
            
            Int GetValue(Internal::ShortIndex i) const { return values[i]; }
            Int GetColumn(Internal::ShortIndex i, int column) const
            {
                for(; column>0; column--)
                    i = Tail(values[i]);
                return values[i];
            }
            
        private:
            std::vector<Int, Alloc> values;
            Internal::HashIndex<Alloc> index;
        };
    }
}
