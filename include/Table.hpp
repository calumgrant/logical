#pragma once
#include "Logical.hpp"
#include <vector>

namespace Logical
{
    template<int Arity>
    struct FixedArity
    {
        const int value = Arity;
    };

    struct VariableArity
    {
        VariableArity(int a) : value(a) {}
        int value;
    };

    inline void read_row(const Int * row, Int & i)
    {
        i = *row;
    }

    template<typename...Ints>
    void read_row(const Int * row, Int & i, Ints&... is)
    {
        i = *row;
        read_row(row+1, is...);
    }

    inline void write_row(Int * row, Int i)
    {
        *row = i;
    }

    template<typename...Ints>
    void write_row(Int * row, Int i, Ints... is)
    {
        *row = i;
        write_row(row+1, is...);
    }

    template<typename Arity>
    void copy_row(Arity arity, Int * target, const Int * source)
    {
        for(int i=0; i<arity.value; ++i)
            target[i] = source[i];
    }

    template<typename Arity>
    void swap_row(Arity arity, Int * target, Int * source)
    {
        for(int i=0; i<arity.value; ++i)
            std::swap(target[i], source[i]);
    }

    // ?? How to swap two rows?
    // ?? How to use std::sort
    template<typename Arity, typename I>
    struct rowref
    {
        I * p;
        Arity a;

        RowRef & operator=(const RowRef&other)
        {
            copy_row(a, p, other.p);
            return *this;
        }

        bool operator<(const rowref & other) const
        {
            for(int i=0; i<a.value; ++i)
            {
                if(p[i]<other.p[i]) return true;
                if(p[i]>other.p[i]) return false;
            }
            return false;
        }

        bool operator==(const rowref & other) const
        {
            for(int i=0; i<a.value; ++i)
            {
                if(p[i]!=other.p[i]) return false;
            }
            return true;
        }
    };

    // An iterator over 
    template<typename Arity, typename I>
    class iterator
    {
    public:
        iterator(I * p, Arity a) : p(p), a(a) {}

        typedef rowref<Arity, I> value_type;

        template<typename...Ints>
        void get(Ints... is) const
        {
            read_row(p, is...);
        }
    
        value_type operator*() const
        {
            return value_type(p, a);
        }

        // random-access stuff
        iterator operator++()
        {
            p += arity.value;
        }

        iterator operator+(Int i) const
        {
            return iterator(p+arity.value * i);
        }

        std::difference_type operator-(const iterator & other) const
        {
            return (p - other.p)/arity.value;
        }

    private:
        I * p;
        Arity a;
    };

    template<typename Arity, typename Int>
    class indexed_iterator
    {

    };

    template<typename Arity>
    class Table
    {
    public:
        Table(int a) : arity(a) {}
        Table() {}

        std::vector<Int> values;

        int get_arity() const { return arity.value; }

        void Finalize();

        typedef Logical::iterator<Arity, Int> iterator;
        typedef Logical::iterator<Arity, cost Int> const_iterator;

        auto begin() const { return const_iterator(values.data(), arity); }
        auto end() const { return const_iterator(values.data() + values.size(), arity); }

        auto begin() { return iterator(values.data(), arity); }
        auto end() { return iterator(values.data() + values.size(), arity); }

    private:
        Arity arity;
        void Sort()
        {
            std::sort(begin(), end());
        }
        void Compact()
        {
            if(size()<=1) return;
            auto e = end();
            auto b = begin();
            auto previous = b;
            for(auto input = begin()+1, output = begin(); input!=e; ++previous, ++input)
            {
                while(*previous == *input) ++input;
            }
        }
    };

    template<int Arity>
    class HashTable : public Table<Arity>
    {
    public:
        void Finalise();
    private:
        std::vector<Int> values;
    };

    template<int Arity>
    class QueryableHashTable : public HashTable<Arity>
    {
        class iterator
        {

        };

        template<typename...Ints>
        iterator Find(Ints...is);
    };

    template<int Arity>
    class SortedTable : Table<Arity>
    {
    public:
        template<int BoundColumns>
        class iterator
        {
            const Int * current, *end;

            template<typename...Ints>
            bool Next(Ints&... outputs);
        };

        iterator<0> Find() const;

        template<typename...Ints>
        auto Find(Ints... vs) -> iterator(sizeof...(vs...))
        {
        }
    };

    /*
        This table simply pushes data onto the end, then
        sorts it and compacts it
    */
    template<int Arity>
    class SimpleTable : public Table<Arity>
    {
    public:
        template<typename... Ints>
        void Add(Ints... v);
    private:
        std::vector<Int> values;
    };
}

namespace std
{
    template<typename A, typename I>
    void swap(rowref<A,I> & r1, rowref<A,I> &b)
    {
        Logical::swap_rows(r1.a, r1.p, r2.p);
    }
}
