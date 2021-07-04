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
    
        template<bool... Bs> struct ConvertMask;
    
        template<> struct ConvertMask<> { static const int value = 0; };
    
        template<bool... Bs> struct ConvertMask<true, Bs...>
        {
            static const int value = 1 + (ConvertMask<Bs...>::value<<1);
        };

        template<bool... Bs> struct ConvertMask<false, Bs...>
        {
            static const int value = ConvertMask<Bs...>::value<<1;
        };

    }

    template<bool...Binding>
    struct StaticBinding
    {
    };

    struct DynamicBinding
    {
        template<bool...Bs>
        explicit DynamicBinding(StaticBinding<Bs...>) :
            mask(Internal::ConvertMask<Bs...>::value), arity(sizeof...(Bs))
        {
            
        }
        
        template<typename...Bools>
        explicit DynamicBinding(Bools... bs) : mask(Internal::MakeMask(bs...)), arity(sizeof...(bs)) {}
        
        DynamicBinding(Int m, int a) : mask(m), arity(a) {}
        
        Int mask;
        int arity;
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

    typedef std::uint32_t ShortIndex;


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

    // Returns the smallest pointer that is > value
    template<typename Arity, typename Binding, typename...Ints>
    ShortIndex UpperBound(Arity a, Binding b, const Int * p, ShortIndex n, Ints... vs)
    {
        ShortIndex l=0, r=n;
        while(l<r)
        {
            auto m = (l+r)>>1;

            if(less(p + m * a.value, vs...))
            {
                r = m;
            }
            else
            {
                l = m+1;
            }
        }
        return a.value*r;
    }



    template<int Arity>
    const Int * upper_bound(const Int * p, Int size)
    {
        // Optimization of previous case
        return p + Arity * size;
    }

    template<typename Arity, typename Binding>
    ShortIndex UpperBound(Arity a, Binding b, const Int * p, ShortIndex n)
    {
        return n * a.value;
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
    void copy_row(Arity arity, const Int * source, Int * target)
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

    inline bool row_equals(const Int *x) { return true; }

    template<typename...Ints>
    bool row_equals(const Int * x, Int y, Ints...ys)
    {
        return *x == y && row_equals(x+1, ys...);
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
        static bool BoundEquals(const Int * x, const Int *y) { return true; }
        static bool BoundLess(const Int * row) { return false; }
        static bool BoundLess(const Int * i, const Int * j) { return false; }
        
        template<typename... Ints>
        static bool BoundGreater(const Int * row, Ints...is) { return false; }

        static void BindRow(const Int * row) { }
        static void BindRow(const Int * row, Int * output) { }

        static const int BindCount = 0;
    };


    template<bool...Binding>
    struct HashHelper<true, Binding...>
    {
        template<typename...Ints>
        static Int Hash(Int i, Ints... is) { return i + P * HashHelper<Binding...>::Hash(is...); }
        static Int Hash(const Int * row) { return *row + P * HashHelper<Binding...>::Hash(row+1); }
        
        template<typename...Ints>
        static bool BoundEquals(const Int * row, Int i, Ints...is) { return i==*row && HashHelper<Binding...>::BoundEquals(row+1, is...); }
        
        static bool BoundEquals(const Int * x, const Int *y) { return *x==*y && HashHelper<Binding...>::BoundEquals(x+1, y+1); }

        template<typename...Ints>
        static bool BoundLess(const Int * row, Int i, Ints...is) { return *row < i || (*row==i && HashHelper<Binding...>::BoundLess(row+1, is...)); }

        static bool BoundLess(const Int * i, const Int * j) { return *i < *j || (*i==*j && HashHelper<Binding...>::BoundLess(i+1, j+1)); }

        template<typename...Ints>
        static bool BoundGreater(const Int * row, Int i, Ints...is) { return *row > i || (*row==i && HashHelper<Binding...>::BoundGreater(row+1, is...)); }

        template<typename...Ints>
        static void BindRow(const Int * row, Int i, Ints&&...is) { HashHelper<Binding...>::BindRow(row+1, is...); }

        static void BindRow(const Int * row, Int * output) { HashHelper<Binding...>::BindRow(row+1, output+1); }
        static const int BindCount = 1 + HashHelper<Binding...>::BindCount;
        
        static void BindRow(const Int * row) { HashHelper<Binding...>::BindRow(row+1); }
    };

    template<bool...Binding>
    struct HashHelper<false, Binding...>
    {
        template<typename...Ints>
        static Int Hash(Int i, Ints... is) { return HashHelper<Binding...>::Hash(is...); }
        static Int Hash(const Int * row) { return HashHelper<Binding...>::Hash(row+1); }

        template<typename...Ints>
        static bool BoundEquals(const Int * row, Int i, Ints...is) { return HashHelper<Binding...>::BoundEquals(row+1, is...); }

        static bool BoundEquals(const Int * x, const Int *y) { return HashHelper<Binding...>::BoundEquals(x+1, y+1); }
        
        template<typename...Ints>
        static bool BoundLess(const Int * row, Int i, Ints...is) { return HashHelper<Binding...>::BoundLess(row+1, is...); }

        static bool BoundLess(const Int * i, const Int * j) { return HashHelper<Binding...>::BoundLess(i+1, j+1); }

        template<typename...Ints>
        static bool BoundGreater(const Int * row, Int i, Ints...is) { HashHelper<Binding...>::BoundGreater(row+1, is...); }

        static bool BoundGreater(const Int * row) { return HashHelper<Binding...>::BoundGreater(row+1); }

        static bool BoundLess(const Int * row) { return HashHelper<Binding...>::BoundGreater(row+1); }
        
        // Bind the unbound columns
        template<typename...Ints>
        static void BindRow(const Int * row, Int & i, Ints&&...is) { i=*row; HashHelper<Binding...>::BindRow(row+1, is...); }

        // Bind the unbound columns
        static void BindRow(const Int * row, Int * output) { *output = *row; HashHelper<Binding...>::BindRow(row+1, output+1); }
        
        static const int BindCount = HashHelper<Binding...>::BindCount;
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

    template<bool... Binding, typename...Ints>
    bool BoundLess(StaticBinding<Binding...>, const Int * row, Ints... is)
    {
        return HashHelper<Binding...>::BoundLess(row, is...);
    }

    template<bool... Binding, typename...Ints>
    bool BoundGreater(StaticBinding<Binding...>, const Int * row, Ints... is)
    {
        return HashHelper<Binding...>::BoundGreater(row, is...);
    }

    template<bool... Binding>
    bool BoundLess(StaticBinding<Binding...>, const Int * i, const Int * j)
    {
        return HashHelper<Binding...>::BoundLess(i, j);
    }

    inline bool BoundLess(DynamicBinding b, const Int *i, const Int *j)
    {
        for(auto m=b.mask; m; m>>=1, ++i, ++j)
        {
            if(m&1)
            {
                if(*i<*j) return true;
                if(*i>*j) return false;
            }
        }
        return false;
    }

    inline bool BoundLess(Int m, const Int *i)
    {
        return false;
    }

    template<typename...Ints>
    bool BoundLess(Int m, const Int *i, Int j, Ints...js)
    {
        if(m&1)
        {
            if(*i<j) return true;
            if(*i>j) return false;
        }
        return BoundLess(m>>1, i+1, js...);
    }

    template<typename...Ints>
    bool BoundLess(DynamicBinding b, const Int *i, Ints...j)
    {
        return BoundLess(b.mask, i, j...);
    }

    inline bool BoundGreater(Int m, const Int *i)
    {
        return false;
    }

    template<typename...Ints>
    bool BoundGreater(Int m, const Int *i, Int j, Ints...js)
    {
        if(m&1)
        {
            if(*i>j) return true;
            if(*i<j) return false;
        }
        return BoundLess(m>>1, i+1, js...);
    }

    template<typename...Ints>
    bool BoundGreater(DynamicBinding b, const Int *i, Ints...j)
    {
        return BoundGreater(b.mask, i, j...);
    }


    template<bool...Binding>
    bool BoundGreater(StaticBinding<Binding...> b, const Int *i, const Int *j)
    {
        return BoundLess(b, j, i);
    }

    inline bool BoundGreater(DynamicBinding b, const Int *i, const Int *j)
    {
        return BoundLess(b, j, i);
    }

    template<bool... Binding>
    bool BoundEquals(StaticBinding<Binding...>, const Int * row1, const Int * row2)
    {
        return HashHelper<Binding...>::BoundEquals(row1, row2);
    }

    inline bool BoundEquals(Int mask, const Int * row) { return true; }

    template<typename...Ints>
    bool BoundEquals(Int mask, const Int * row, Int i, Ints... is)
    {
        return mask && (!(mask&1) || i == *row) && BoundEquals(mask>>1, row+1, is...);
    }

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
    void BindRow(StaticBinding<Binding...>, const Int * row, Ints&&... is)
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
    void BindRow(Int mask, const Int * row, Int &i, Ints&&... is)
    {
        if(0==(mask&1)) i=*row;
        BindRow(mask>>1, row+1, is...);
    }

/*
    template<typename...Ints>
    void BindRow(Int mask, const Int * row, Int i, Ints... is)
    {
        BindRow(mask>>1, row+1, is...);
    }
*/

    template<typename...Ints>
    void BindRow(DynamicBinding b, const Int * row, Ints&&... is)
    {
        BindRow(b.mask, row, is...);
    }

    inline void BindRow(DynamicBinding b, const Int * row, Int * output)
    {
        int a=0;
        for(auto m=b.mask; a<b.arity; m>>=1, a++, row++, output++)
        {
            if(!(m&1)) *output = *row;
        }
    }

    inline DynamicArity Count(DynamicBinding b)
    {
        int c=0;
        for(int m=b.mask; m; m>>=1)
            if(m&1) c++;
        return c;
    }

    template<bool...Binding>
    constexpr auto Count(StaticArity<Binding...> arity) -> StaticBinding<HashHelper<Binding...>::BindCount>
    {
        return StaticBinding<HashHelper<Binding...>::BindCount>();
    }

    // Returns the smallest pointer that is > value
    template<typename Arity, typename Binding>
    ShortIndex UpperBound(Arity a, Binding b, const Int * p, ShortIndex n, const Int* vs)
    {
        ShortIndex l=0, r=n;
        while(l<r)
        {
            auto m = (l+r)>>1;

            if(BoundGreater(b, p + m * a.value, vs))
            {
                r = m;
            }
            else
            {
                l = m+1;
            }
        }
        return a.value*r;
    }

    template<typename Arity, typename Binding, typename... Ints>
    ShortIndex LowerBound(Arity a, Binding b, const Int *p, ShortIndex n, Ints... vs)
    {
        ShortIndex l=0, r=n;
        while(l<r)
        {
            auto m = (l+r)>>1;
                        
            if (BoundLess(b, p + m*a.value, vs...))
            {
                l = m+1;
            }
            else
            {
                r = m;
            }
        }
        return l*a.value;
    }


    template<typename Arity, typename Binding>
    ShortIndex LowerBound(Arity a, Binding b, const Int *p, ShortIndex n, const Int * vs)
    {
        ShortIndex l=0, r=n;
        while(l<r)
        {
            auto m = (l+r)>>1;
                        
            if (BoundLess(b, p + m*a.value, vs))
            {
                l = m+1;
            }
            else
            {
                r = m;
            }
        }
        return l*a.value;
    }

    template<typename Arity, typename Binding>
    ShortIndex LowerBound(Arity a, Binding b, const Int *p, ShortIndex n)
    {
        return 0;
    }

    inline DynamicBinding GetBoundBinding(DynamicArity a)
    {
        return DynamicBinding(Int(1<<a.value)-1, a.value);
    }

    inline DynamicBinding GetUnboundBinding(DynamicArity a)
    {
        return DynamicBinding(Int(0), a.value);
    }

    template<bool b, typename SB> struct BindingCons;

    template<bool b, bool...bs>
    struct BindingCons<b, StaticBinding<bs...>>
    {
        typedef StaticBinding<b, bs...> type;
    };

    template<int Arity>
    struct MakeBinding
    {
        typedef typename BindingCons<true, typename MakeBinding<Arity-1>::bound>::type bound;
        typedef typename BindingCons<false, typename MakeBinding<Arity-1>::unbound>::type unbound;
    };

    template<> struct MakeBinding<0>
    {
        typedef StaticBinding<> bound;
        typedef StaticBinding<> unbound;
    };

    template<int Arity>
    auto GetBoundBinding(StaticArity<Arity>) { return typename MakeBinding<Arity>::bound(); }

    template<int Arity>
    auto GetUnboundBinding(StaticArity<Arity>) { return typename MakeBinding<Arity>::unbound(); }

    template<typename T> struct BoundTypeFromArity;
    template<typename T> struct UnboundTypeFromArity;

    template<> struct BoundTypeFromArity<DynamicArity>
    {
        typedef DynamicBinding type;
    };

    template<> struct UnboundTypeFromArity<DynamicArity>
    {
        typedef DynamicBinding type;
    };

    template<int A> struct BoundTypeFromArity<StaticArity<A>>
    {
        typedef typename MakeBinding<A>::bound type;
    };

    template<int A> struct UnboundTypeFromArity<StaticArity<A>>
    {
        typedef typename MakeBinding<A>::unbound type;
    };
}
}
