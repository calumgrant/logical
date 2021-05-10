
#include <memory>
#include <unordered_map>
#include <unordered_set>

#include "AST.hpp"

class Evaluation;

std::shared_ptr<Evaluation> CompileRule(AST::Clause &lhs, AST::Clause &rhs);

// Compiling a predicate
class Compilation
{
public:
    Compilation();
    Compilation(const Compilation&) = delete;
    ~Compilation();

    int AddVariable(int name, bool & alreadyBound);
    int AddValue(const Entity &e);
    int AddUnnamedVariable();
    
    int CreateBranch();
    void Branch(int branch);
    
    std::vector<Entity> row;
private:
    
    // The stack of bound variables, that can be rewound.
    std::vector<int> boundVariables;

    // The variables bound in the current branch
    std::unordered_set<int> boundVariables2;

    // This only grows
    std::unordered_map<int, int> variables;
};
