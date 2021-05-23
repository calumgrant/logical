#include <memory>
#include <utility>

class Database;
class Entity;
class Compilation;
class Evaluation;
class CompoundName;
class Relation;
class OptimizationOptions;
class RecursiveLoop;

typedef std::shared_ptr<Evaluation> EvaluationPtr;

typedef int EntityId;
typedef int StringId;
typedef int Arity;
typedef int RelationId;
typedef int VariableId;
typedef int ColumnMask;

typedef std::size_t Size;
typedef std::size_t RowIndex;
