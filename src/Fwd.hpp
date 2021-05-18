#include <memory>
#include <utility>

class Database;
class Entity;
class Compilation;
class Evaluation;
class CompoundName;
class Relation;

typedef std::shared_ptr<Evaluation> EvaluationPtr;

typedef int EntityId;
typedef int StringId;
typedef int Arity;
typedef int RelationId;
typedef int VariableId;
