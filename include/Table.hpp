#pragma once

#include "Logical.hpp"
#include "Utils.hpp"

#include <vector>
#include <iostream>
#include <algorithm>

namespace Logical
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

    template<typename Arity, typename Alloc = std::allocator<Int>>
    class Table
    {
    public:
        Table(Arity a) : arity(a) {}
        Table() {}

        std::vector<Int, Alloc> values;
        Arity arity;

        int get_arity() const { return arity.value; }
        
        Int size() const { return values.size()/arity.value; }

    protected:
        
        void Compact()
        {
            values = CompactTable(arity, values);
        }
    };


    struct Segment
    {
        const Int * first, * last;
    };

    template<typename Arity, typename Bound, typename...Ints>
    bool Next(Segment &s, Ints... is);

    template<typename Arity>
    class SortedTable : public Table<Arity>
    {
    public:
        SortedTable(Table<Arity> && src) : Table<Arity>(src.arity)
        {
            this->values = CompactTable(src.arity, src.values);
        }

        template<int BoundColumns>
        class enumerator
        {
        public:
            enumerator(Arity a, const Int * x, const Int *y) : arity(a), current(x), end(y) {}

            template<typename...Ints>
            bool Next(Ints&... outputs)
            {
                auto old_current = current;
                if(current < end)
                {
                    Internal::read_row(current+BoundColumns, outputs...);
                    current += arity.value;
                    
                    if(BoundColumns + sizeof...(outputs) < arity.value)
                    {
                        while(current<end && Internal::row_equals(arity, old_current, current))
                            current += arity.value;
                    }
                    
                    return true;
                }
                return false;
            }
        private:
            Arity arity;
            const Int * current, *end;
        };
        
        enumerator<0> Find() const
        {
            return enumerator<0>{this->arity, &*this->values.begin(), &*this->values.end()};
        }
        
        template<typename...Ints>
        auto Find(Int a, Ints... vs) const -> enumerator<1+sizeof...(vs)>
        {
        }
        
        template<typename Bound, typename...Ints>
        auto Find(Bound b, Ints...is)
        {
        }
        
        template<typename Bound>
        auto Find(Bound b, const Int * row);
        
        template<typename Bound, int E>
        bool Next(Bound b, enumerator<E> e, Int * row);
    };

    /*
        This table simply pushes data onto the end, then
        sorts it and compacts it
    */
    template<typename Arity>
    class SimpleTable : public Table<Arity>
    {
    public:
        SimpleTable() {}
        SimpleTable(int a) : Table<Arity>(a) {}
        
        template<typename... Ints>
        void Add(Ints... v)
        {
            // static_assert(sizeof...(v) % Arity::value == 0);
            AddInternal(v...);
        }
        
        void Finalize()
        {
            this->Compact();
        }

    private:
        void AddInternal(Int a) { this->values.push_back(a); }

        template<typename... Ints>
        void AddInternal(Int a, Ints... v)
        {
            this->values.push_back(a);
            AddInternal(v...);
        }
    };


    template<typename Arity>
    class ExternSortedTable
    {
    public:
        Arity arity;
        Segment data;

        typedef Segment enumerator;
        
        enumerator Find() const
        {
            return data;
        }
        
        template<typename... Ints>
        bool Next(enumerator & e, Ints... is)
        {
            if(e.first < e.last)
            {
                Internal::read_row(e.first, is...);
                e.first += arity.value;
                return true;
            }
        }
    };

    template<typename Arity>
    class ExternTable
    {
        Segment data;
    };
}

