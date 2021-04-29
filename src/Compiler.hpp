
#include <memory>
#include "AST.hpp"

class Evaluation;

std::shared_ptr<Evaluation> CompileRule(AST::Clause &lhs, AST::Clause &rhs);
