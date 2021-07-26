#pragma once

#include "Index.hpp"

#include <vector>
#include <algorithm>

namespace Logical
{
    namespace Internal
    {
        template<typename Arity, typename Alloc>
        std::vector<Int,Alloc> CompactTable(Arity arity, const std::vector<Int,Alloc> & values)
        {
            auto s = values.size()/arity.value;
            std::vector<int> indexes(s);
            for(int i=0; i<s; ++i)
                indexes[i] = i * arity.value;
            
            std::sort(indexes.begin(), indexes.end(), [&](int a, int b) { return Internal::row_less(arity, &values[a], &values[b]); });

            std::vector<Int,Alloc> results(values.get_allocator());
            results.reserve(values.size());
            
            // Now, mark duplicates
            for(int i=0; i<s; ++i)
                if(i==0 || !Internal::row_equals(arity, &values[indexes[i-1]], &values[indexes[i]]))
                {
                    auto p = &values[indexes[i]];
                    for(int i=0; i<arity.value; ++i)
                        results.push_back(p[i]);
                }
            
            return std::move(results);
        }

        // Specialisation of previous case.
        template<typename Alloc>
        std::vector<Int,Alloc> CompactTable(StaticArity<1>, std::vector<Int,Alloc> & values)
        {
            std::sort(values.begin(), values.end());
            Int out=0;
            
            for(Int in=0; in < values.size(); ++in)
            {
                if(in==0 || values[in-1] != values[in])
                    values[out++] = values[in];
            }
            
            values.resize(out);
            
            return std::move(values);
        }
    }

    template<typename Arity, typename Alloc = std::allocator<Int>>
    class Table
    {
    protected:
        Table(Arity a, Alloc alloc = Alloc()) : arity(a), values(alloc) {}

        template<typename Alloc2>
        Table(const Table<Arity, Alloc2> & src, Alloc alloc = Alloc()) : arity(src.arity), values(src.values.begin(), src.values.end(), alloc) {}

    public:
        int get_arity() const { return arity.value; }
        
        Int size() const { return values.size()/arity.value; }

        std::vector<Int, Alloc> values;
        Arity arity;
        
        void clear() { values.clear(); }
    };

    template<typename Arity, typename Alloc = std::allocator<Int>>
    class CompressedTable
    {
        std::vector<int, Alloc> columnTypes;
        std::vector<std::uint32_t, Alloc> data;
    };

    template<typename Arity>
    class SortedTable : public Table<Arity>
    {
    public:
        SortedTable(Table<Arity> && src) : Table<Arity>(src.arity)
        {
            this->values = Internal::CompactTable(src.arity, src.values);
        }

        SortedIndex<Arity> GetIndex() const
        {
            return SortedIndex<Arity>(this->arity, this->values.data(), this->values.size()/this->arity.value);
        }
    };

    /*
        This table simply pushes data onto the end, then
        sorts it and compacts it
    */
    template<typename Arity>
    class BasicTable : public Table<Arity>
    {
    public:
        BasicTable(Arity a = Arity()) : Table<Arity>(a) {}
        
        template<typename... Ints>
        void Add(Ints... v)
        {
            AddInternal(v...);
        }
        
        template<typename Int>
        void Add(Int * data)
        {
            this->values.insert(this->values.end(), data, data+this->arity.value);
        }
        
        UnsortedIndex<Arity> GetIndex() const
        {
            return UnsortedIndex<Arity>(this->arity, this->values.data(), this->values.size());
        }

    private:
        void AddInternal(Int a) { this->values.push_back(a); }

        template<typename... Ints>
        void AddInternal(Int v, Ints... vs)
        {
            this->values.push_back(v);
            AddInternal(vs...);
        }
    };
}

