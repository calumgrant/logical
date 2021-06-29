#pragma once
#include "Logical.hpp"
#include <utility>
#include <iterator>
#include <iostream>

namespace Logical
{
namespace Internal
{
    class Writer
    {
    };

    inline bool less(const Int*row, Int v);
    inline bool greater(const Int*row, Int v);

    template<typename...Ints>
    bool less(const Int*row, Int v, Ints... vs)
    {
        return v < *row || (v == *row && less(row+1, vs...));
    }

    template<typename...Ints>
    bool greater(const Int*row, Int v, Ints... vs)
    {
        return v > *row || (v == *row && greater(row+1, vs...));
    }

    inline bool less(const Int*row, Int v)
    {
        return v < *row;
    }
    
    bool greater(const Int*row, Int v)
    {
        return v > *row;
    }
    
    // Returns the highest pointer that is <= value
    template<int Arity, typename ... Ints>
    const Int * lower_bound(const Int * p, Int n, Ints ... vs)
    {
        Int l=0, r=n;
        while(l<r)
        {
            auto m = (l+r)>>1;
                        
            if (greater(p + m*Arity, vs...))
            {
                l = m+1;
            }
            else
            {
                r = m;
            }
        }
        return p + l*Arity;
    }
    
    template<int Arity>
    const Int * lower_bound(const Int * p, Int n)
    {
        // Optimization of previous case
        return p;
    }

    // Returns the smallest pointer that is > value
    template<int Arity, typename...Ints>
    const Int * upper_bound(const Int * p, Int n, Ints... vs)
    {
        Int l=0, r=n;
        while(l<r)
        {
            auto m = (l+r)>>1;

            if(less(p + m * Arity, vs...))
            {
                r = m;
            }
            else
            {
                l = m+1;
            }
        }
        return p+Arity*r;
    }
    
    template<int Arity>
    const Int * upper_bound(const Int * p, Int size)
    {
        // Optimization of previous case
        return p + Arity * size;
    }

    inline void read_row(const Int *p) {}
    inline void read_row(const Int *p, Int &v) { v = *p; }

    template<typename...Ints>
    void read_row(const Int *p, Int &v, Ints&...vs) { v = *p; read_row(p+1, vs...); }

    inline bool equals_row(const Int *p) { return true; }
    inline bool equals_row(const Int *p, Int v) { return v == *p; }

    template<typename...Ints>
    bool equals_row(const Int *p, Int v, Ints... vs) { return v == *p && equals_row(p+1, vs...); }

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
    void swap_rows(Arity arity, Int * target, Int * source)
    {
        for(int i=0; i<arity.value; ++i)
            std::swap(target[i], source[i]);
    }
}



// ?? How to swap two rows?
// ?? How to use std::sort
template<typename Arity, typename I>
struct rowref
{
    I * p;
    Arity a;

    rowref & operator=(const rowref&other)
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


// An iterator pointing to a row in a table
// Deleteme
template<typename Arity, typename I>
class row_iterator
{
public:
    rowref<Arity, I> rr;
    
    row_iterator(I * p, Arity a) : rr{p,a} {}

    typedef Int size_type;
    typedef Int difference_type;
    typedef rowref<Arity, I> value_type;
    typedef value_type * pointer;
    typedef value_type reference;
    typedef std::random_access_iterator_tag iterator_category;
    
    template<typename...Ints>
    void get(Ints... is) const
    {
        read_row(rr.p, is...);
    }

    reference operator*()
    {
        return rr;
    }
    
    pointer operator->()
    {
        return &rr;
    }

    const value_type * operator->() const
    {
        return &rr;
    }
    
    // random-access stuff
    row_iterator & operator++()
    {
        rr.p += rr.a.value;
        return *this;
    }

    row_iterator &operator--()
    {
        rr.p -= rr.a.value;
        return *this;
    }

    row_iterator & operator+=(Int i)
    {
        rr.p += i * rr.a.value;
        return *this;
    }
    
    row_iterator operator+(Int i) const
    {
        return row_iterator(rr.p+rr.a.value * i, rr.a);
    }

    Int operator-(const row_iterator & other) const
    {
        return (rr.p - other.rr.p)/rr.a.value;
    }
    
    bool operator==(const row_iterator & other) const
    {
        return rr.p == other.rr.p;
    }

    bool operator!=(const row_iterator & other) const
    {
        return rr.p != other.rr.p;
    }

    bool operator>=(const row_iterator & other) const
    {
        return rr.p >= other.rr.p;
    }

    bool operator<(const row_iterator & other) const
    {
        return rr.p < other.rr.p;
    }

    bool operator>(const row_iterator & other) const
    {
        return rr.p > other.rr.p;
    }
};

template<typename A, typename I>
std::ostream & operator<<(std::ostream & os, const row_iterator<A,I> & rr)
{
    return os << "(" << rr->p << "...)";
}

template<typename Arity>
bool row_less(Arity a, const Int * x, const Int *y)
{
    for(int i=0; i<a.value; ++i)
    {
        if(x[i] < y[i]) return true;
        if(x[i] > y[i]) return false;
    }
    return false;
}

template<typename Arity>
bool row_equals(Arity a, const Int * x, const Int *y)
{
    for(int i=0; i<a.value; ++i)
    {
        if(x[i] != y[i]) return false;
    }
    return true;
}


}
