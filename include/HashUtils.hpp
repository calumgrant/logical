#pragma once
#include "Logical.hpp"

namespace Logical
{

namespace Internal
{
    static const ShortIndex EmptyTupleHash = 0;
    static const ShortIndex EmptyCell = -1;

    inline ShortIndex Hash0(Int i) { return i ^ (i>>32); }

    inline ShortIndex HashCombine(ShortIndex seed, Int v)
    {
        return seed ^ (Hash0(v) + 0x9e3779b9 + (seed<<6) + (seed>>2));
    }

    template<typename Arity>
    ShortIndex Hash(Arity) { return EmptyTupleHash; }

    template<typename Arity, typename...Ints>
    ShortIndex Hash(Arity a, Int i, Ints... is)
    {
        return HashCombine(Hash(a, is...), i);
    }

    template<bool...Binding>
    struct HashHelper;

    template<>
    struct HashHelper<>
    {
        static ShortIndex Hash() { return EmptyTupleHash; }
        static ShortIndex Hash(const Int * row) { return EmptyTupleHash; }
    };

    template<bool...Binding>
    struct HashHelper<true, Binding...>
    {
        template<typename...Ints>
        static ShortIndex Hash(Int i, Ints... is) { return HashCombine(HashHelper<Binding...>::Hash(is...), i); }
        static ShortIndex Hash(const Int * row) { return HashCombine(HashHelper<Binding...>::Hash(row+1), *row); }
    };

    template<bool...Binding>
    struct HashHelper<false, Binding...>
    {
        template<typename...Ints>
        static ShortIndex Hash(Int i, Ints... is) { return HashHelper<Binding...>::Hash(is...); }
        static ShortIndex Hash(const Int * row) { return HashHelper<Binding...>::Hash(row+1); }
    };

    template<bool...Bound, typename...Ints>
    ShortIndex BoundHashI(Ints... is) { return HashHelper<Bound...>::Hash(is...); }

    template<bool...Bound>
    ShortIndex BoundHashI(const Int * row) { return HashHelper<Bound...>::Hash(row); }

    inline ShortIndex BoundHash(DynamicBinding b, const Int * row)
    {
        Int h=EmptyTupleHash;
        for(int i=b.arity.value-1; i>=0; --i)
        {
            if(b.mask & (1<<i)) { h = HashCombine(h, row[i]); }
        }
        return h;
    }

    template<bool...Bs>
    ShortIndex BoundHash(StaticBinding<Bs...> b, const Int * row)
    {
        return HashHelper<Bs...>::Hash(row);
    }

    template<bool...Bs, typename...Ints>
    ShortIndex BoundHash(StaticBinding<Bs...> b, Ints...is)
    {
        return HashHelper<Bs...>::Hash(is...);
    }

    inline ShortIndex HashWithMask(Int m) { return EmptyTupleHash; }

    template<typename...Ints>
    ShortIndex HashWithMask(Int m, Int i, Ints...is)
    {
        return (m&1)? HashCombine(HashWithMask(m>>1, is...), i) : HashWithMask(m>>1, is...);
    }

    template<typename...Ints>
    ShortIndex BoundHash(DynamicBinding b, Int i, Ints...is)
    {
        return HashWithMask(b.mask, i, is...);
    }

    template<typename Arity>
    ShortIndex Hash(Arity arity, const Int *p)
    {
        Int h = EmptyTupleHash;
        for(int i=arity.value-1; i>=0; --i)
            h = HashCombine(h, p[i]);
        return h;
    }

    inline void FirstHash(Enumerator & e, ShortIndex h)
    {
        e.i = h;
        e.j = 1;
    }

    inline void NextHash(Enumerator &e)
    {
        ++e.j;
    }

    inline ShortIndex GetIndex(const Enumerator &e, ShortIndex s)
    {
        return (e.i + e.j * e.j) % s;
    }

    ShortIndex GetIndexSize(int index);
}
}
