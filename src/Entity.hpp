#pragma once

typedef int EntityId;

enum class EntityType
{
    None,
    Integer,
    String,
    Float,
    AtString,
    Boolean,
    Char,
    Byte
};

// Something that's stored in the database
struct Entity
{
    EntityType type;
    union
    {
        int i;
        float f;
        char ch[4];
    };

    Entity() : type(EntityType::None) { }
    Entity(EntityType t, int i) : type(t), i(i) { }
    Entity(EntityType t, float f) : type(t), f(f) { }

    bool operator==(const Entity & other) const { return type == other.type && i == other.i; }

    int hash() const { return (int)type * 0x10001 + i; }

    bool operator<(const Entity & other) const 
    { 
        if(type == other.type)
        {
            return type == EntityType::Float ? f < other.f : i < other.i;
        }

        return type < other.type;
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
        switch(type)
        {
            case EntityType::None:
                *this = other;
                break;
            case EntityType::Integer:
                switch(other.type)
                {
                    case EntityType::Integer:
                        i += other.i;
                        break;
                    case EntityType::Float:
                        type = EntityType::Float;
                        f = i + other.f;
                        break;
                    default:
                        break;
                }
                break;
            case EntityType::Float:
                switch(other.type)
                {
                    case EntityType::Integer:
                        f += other.i;
                        break;
                    case EntityType::Float:
                        f += other.f;
                        break;
                    default:
                        break;
                }
                break;
            default:
                break;
        }
        
        // Anything else fails silently.
        // We should perhaps warn about this instead.
        return *this;
    }
};

struct PairHash
{
    int operator()(const std::pair<Entity, Entity> &value) const
    {
        Entity::Hash h;
        return h(value.first) * 13 + h(value.second);
    }
};

