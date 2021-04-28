
#include <memory>
#include "AST.hpp"

class Evaluation;

std::shared_ptr<Evaluation> Compile(AST::Clause &clause);