#include <string>
#include <iostream>

namespace AST
{
    class Clause
    {
    };

    class UnaryRelation : public Clause
    {

    };

    class BinaryRelation : public Clause
    {

    };

    class Variable : public Clause
    {

    };

    class UnderscoreVariable : public Variable
    {

    };

    class Entity : public Clause
    {
    };

    class AtEntity : public Entity
    {
    };

    class String : public Entity
    {
    public:
        String(const std::string &p) : value(p) 
        { 
        }

        std::string value;
    };

    class Integer : public Entity { };

    class Float : public Entity { };

    class Has : public Clause
    {
    };

    class UnaryPredicateList : public Clause { };
}
