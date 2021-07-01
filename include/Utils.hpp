#pragma once
#include "Logical.hpp"
#include <utility>
#include <iterator>
#include <iostream>

namespace Logical
{
    template<int Arity>
    struct StaticArity
    {
        static const int value = Arity;
    };

    struct DynamicArity
    {
        DynamicArity(int a) : value(a) {}
        int value;
    };

    namespace Internal
    {
        inline Int MakeMask() { return 0; }
        
        template<typename...Bs>
        Int MakeMask(bool b, Bs... bs)
        {
            return (Int)b + (MakeMask(bs...)<<1);
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
    inline bool less(const Int*row, Int v)
    {
        return v < *row;
    }

    inline bool greater(const Int*row, Int v)
    {
        return v > *row;
    }

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
        static bool BoundEquals(const Int * row) { return true; }

        static void BindRow(const Int * row) { }
        static void BindRow(const Int * row, Int * output) { }
    };


    template<bool...Binding>
    struct HashHelper<true, Binding...>
    {
        template<typename...Ints>
        static Int Hash(Int i, Ints... is) { return i + P * HashHelper<Binding...>::Hash(is...); }
        static Int Hash(const Int * row) { return *row + P * HashHelper<Binding...>::Hash(row+1); }
        template<typename...Ints>
        static bool BoundEquals(const Int * row, Int i, Ints...is) { return i==*row && HashHelper<Binding...>::BoundEquals(row+1, is...); }
        template<typename...Ints>
        static void BindRow(const Int * row, Int &i, Ints...is) { i=*row; HashHelper<Binding...>::BindRow(row+1, is...); }

        static void BindRow(const Int * row, Int * output) { *output = *row; HashHelper<Binding...>::BindRow(row+1, output+1); }
    };

    template<bool...Binding>
    struct HashHelper<false, Binding...>
    {
        template<typename...Ints>
        static Int Hash(Int i, Ints... is) { return HashHelper<Binding...>::Hash(is...); }
        static Int Hash(const Int * row) { return HashHelper<Binding...>::Hash(row+1); }

        template<typename...Ints>
        static bool BoundEquals(const Int * row, Int i, Ints...is) { return HashHelper<Binding...>::BoundEquals(row+1, is...); }

        template<typename...Ints>
        static void BindRow(const Int * row, Int i, Ints...is) { HashHelper<Binding...>::BindRow(row+1, is...); }

        static void BindRow(const Int * row, Int * output) { HashHelper<Binding...>::BindRow(row+1, output+1); }
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
        return (m&1)? i + P * HashWithMask(m>>1, is...) : HashWithMask(m>>1, is...);
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

    // Compares two rows, but only on the bound columns
    template<bool... Binding, typename...Ints>
    bool BoundEquals(StaticBinding<Binding...>, const Int * row, Ints... is)
    {
        return HashHelper<Binding...>::BoundEquals(row, is...);
    }

    template<bool... Binding>
    bool BoundEquals(StaticBinding<Binding...>, const Int * row1, const Int * row2)
    {
        return HashHelper<Binding...>::BoundEquals(row1, row2);
    }

    template<typename...Ints>
    bool BoundEquals(Int mask, const Int * row, Int i, Ints... is)
    {
        return mask && (!(mask&1) || i == *row) && BoundEquals(mask>>1, row+1, is...);
    }

    inline bool BoundEquals(Int mask, const Int * row) { return true; }

    template<typename...Ints>
    bool BoundEquals(DynamicBinding b, const Int * row, Ints... is)
    {
        return BoundEquals(b.mask, row, is...);
    }

    inline bool BoundEquals(DynamicBinding b, const Int * row1, const Int * row2)
    {
        for(Int m = b.mask; m; m>>=1, row1++, row2++)
        {
            if((m&1) && *row1 != *row2) return false;
        }
        return true;
    }

    template<bool... Binding, typename...Ints>
    void BindRow(StaticBinding<Binding...>, const Int * row, Ints... is)
    {
        HashHelper<Binding...>::BindRow(row, is...);
    }

    template<bool... Binding>
    void BindRow(StaticBinding<Binding...>, const Int * row, Int * output)
    {
        HashHelper<Binding...>::BindRow(row, output);
    }

    inline void BindRow(Int mask, const Int * row) {}

    template<typename...Ints>
    void BindRow(Int mask, const Int * row, Int &i, Ints... is)
    {
        if(mask&1) i=*row;
        BindRow(mask>>1, row+1, is...);
    }

    template<typename...Ints>
    void BindRow(Int mask, const Int * row, Int i, Ints... is)
    {
        BindRow(mask>>1, row+1, is...);
    }

    template<typename...Ints>
    void BindRow(DynamicBinding b, const Int * row, Ints... is)
    {
        BindRow(b.mask, row, is...);
    }

    inline void BindRow(DynamicBinding b, const Int * row, Int * output)
    {
        for(auto m=b.mask; m; m>>=1, row++, output++)
        {
            if(m&1) *output = *row;
        }
    }
}


}
