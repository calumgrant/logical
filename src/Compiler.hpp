
#include <memory>
#include <unordered_map>

#include "AST.hpp"

class Evaluation;

std::shared_ptr<Evaluation> CompileRule(AST::Clause &lhs, AST::Clause &rhs);

// Compiling a predicate
class Compilation
{
public:
    Compilation();
    ~Compilation();
    int AddVariable(const std::string &name, bool & alreadyBound);
    int AddValue(const Entity &e);
    int AddUnnamedVariable();
private:
    int stack_size;
    std::unordered_map<std::string, int> variables;
};
