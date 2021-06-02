#pragma once
#include "Fwd.hpp"

enum class EntityType : std::uint16_t
{
    SilentNan = 0xf8ff,
    None = SilentNan,
    Integer = 0xf9ff,
    String = 0xfaff,
    Float = 0x0000,
    NewType = 0xfbff,
    AtString = 0xfcff,
    Boolean = 0xfdff,
    Char = 0xfeff,
    Byte = 0xffff
};

// Something that's stored in the database
class Entity
{
public:
    EntityType Type() const
    {
        return ((std::uint16_t)integral.type & 0xfff8) == 0xfff8 ?
            EntityType::Float : integral.type;
    }
    
    bool IsInt() const { return integral.type == EntityType::Integer; }
    bool IsFloat() const { return ((std::uint16_t)integral.type & 0xfff8) == 0xfff8; }
    bool IsNone() const { return integral.type == EntityType::None; }
    bool IsString() const { return integral.type == EntityType::String; }
    
    Entity() : integral {EntityType::None,0} { }
    Entity(EntityType t, std::int64_t i) : integral { t, i } { }
    Entity(EntityType t, std::int32_t i) : integral { t, i } { }
    Entity(EntityType t, double d) : d(d) { }

    operator double() const { return d; }
    operator std::int64_t() const { return integral.value; }
    
    void assign(std::int64_t value)
    {
        integral.value = value;
    }

    Entity & operator=(double d) { this->d = d; return *this; }
    Entity & operator=(std::uint64_t v) { integral.value = v; return *this; }

    bool operator==(const Entity & other) const { return i64 == other.i64; }
    bool operator!=(const Entity & other) const { return !(*this == other); }

    int hash() const { return i32[0] + i32[1]; }

    bool operator<(const Entity & other) const 
    { 
        return i64 < other.i64;
    }

    struct Hash
    {
        int operator()(const Entity &e) const
        {
            return e.hash();
        }
    };
    
    Entity & operator+=(const Entity & other)
    {
        switch(integral.type)
        {
            case EntityType::None:
                *this = other;
                break;
            case EntityType::Integer:
                switch(other.integral.type)
                {
                    case EntityType::Integer:
                        integral.value += other.integral.value;
                        break;
                    default:
                        d = integral.value + other.d;
                        break;
                }
                break;
            default:
                // Float
                switch(other.integral.type)
                {
                    case EntityType::Integer:
                        d += other.integral.value;
                        break;
                    default:
                        d += other.d;
                        break;
                }
                break;
        }
        
        // Anything else fails silently.
        // We should perhaps warn about this instead.
        return *this;
    }
private:
    struct IntegralType
    {
        EntityType type : 16;
        std::int64_t value : 48;
    };

    union
    {
        IntegralType integral;
        std::uint64_t i64;
        std::uint32_t i32[2];
        double d;
        char ch[8];
    };
};

struct PairHash
{
    int operator()(const std::pair<Entity, Entity> &value) const
    {
        Entity::Hash h;
        return h(value.first) * 13 + h(value.second);
    }
};

