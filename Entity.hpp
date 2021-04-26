

typedef int EntityId;

enum class EntityType
{
    Integer,
    String,
    Float,
    AtEntity,
    Boolean
};

// Something that's stored in the database
struct Entity
{
    EntityType type;
    union
    {
        int i;
        float f;
    };

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
};
