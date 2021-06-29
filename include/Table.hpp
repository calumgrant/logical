#pragma once

#include "Logical.hpp"
#include "Utils.hpp"

#include <vector>
#include <iostream>
#include <algorithm>

namespace Logical
{
    template<int Arity>
    struct FixedArity
    {
        static const int value = Arity;
    };

    struct VariableArity
    {
        VariableArity(int a) : value(a) {}
        int value;
    };

    template<typename Arity>
    class Table
    {
    public:
        Table(int a) : arity(a) {}
        Table() {}

        std::vector<Int> values;
        Arity arity;

        int get_arity() const { return arity.value; }
        
        Int size() const { return values.size()/arity.value; }

    protected:
        
        void Compact()
        {
            // TODO: Optimize for the case where arity=1.
            auto s = size();
            std::vector<int> indexes(s);
            for(int i=0; i<s; ++i)
                indexes[i] = i * arity.value;
            
            std::sort(indexes.begin(), indexes.end(), [&](int a, int b) { return row_less(arity, &values[a], &values[b]); });

            std::vector<Int> results;
            results.reserve(values.size());
            
            // Now, mark duplicates
            for(int i=0; i<s; ++i)
                if(i==0 || !row_equals(arity, &values[indexes[i-1]], &values[indexes[i]]))
                {
                    auto p = &values[indexes[i]];
                    for(int i=0; i<arity.value; ++i)
                        results.push_back(p[i]);
                }
            
            values.swap(results);
        }
    };

    template<typename Arity>
    class HashTable : public Table<Arity>
    {
    public:
        void Finalise();
    private:
        std::vector<Int> values;
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

    template<typename Arity>
    class SortedTable : Table<Arity>
    {
    public:
        SortedTable(const Table<Arity> & src)
        {
        }
        
        template<int BoundColumns>
        class enumerator
        {
            const Int * current, *end;

            template<typename...Ints>
            bool Next(Ints&... outputs);
        };
        
        template<>
        class enumerator<0>
        {
            template<typename...Ints>
            bool Next(Ints&... outputs);
        };

        enumerator<0> Find() const;

        template<typename...Ints>
        auto Find(Ints... vs) -> enumerator<sizeof...(vs)>
        {
        }
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
            static_assert(sizeof...(v) % Arity::value == 0);
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
}

