#pragma once

class Columns
{
public:
    Columns(std::uint64_t mask) : mask(mask) { }
    
    bool IsBound(int col) const { return mask & (1<<col); }
    
    bool IsUnbound() const { return mask == 0; }
    
    bool IsFullyBound(int arity) const { auto m = (1UL<<arity)-1; return (mask & m) == m; }
    
    void Bind(int col) { mask |= (1UL<<col); }

    struct Hash
    {
        int operator()(Columns c) const { return c.mask; }
    };
    
    struct EqualTo
    {
        bool operator()(Columns c1, Columns c2) const { return c1.mask == c2.mask; }
    };
    
    bool operator<=(Columns other) const
    {
        return (mask & other.mask) == mask;
    }
    
    Columns operator-(Columns other) const
    {
        return mask - other.mask;
    }

    std::uint64_t mask;
};
