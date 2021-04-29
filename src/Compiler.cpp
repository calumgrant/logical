#include "Compiler.hpp"
#include "Database.hpp"

#include <iostream>

class VariableNumberer : public AST::Visitor
{
public:
    std::unordered_map<std::string, int> variables;
    void OnVariable(const std::string &name) override
    {
        variables.insert(std::make_pair(name, variables.size()));
    }
};

void AST::Rule::Compile(Database &db)
{
    int locals_size=0;
    std::unordered_map<std::string, int> variables;

    std::vector<Entity> locals;
    VariableNumberer numberer;

    Visit(numberer);

    std::cout << "Rule has " << numberer.variables.size() << " variables\n";
}
