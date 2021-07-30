#include <memory>
#include <utility>

class Database;
class Entity;
class Compilation;
class Evaluation;
class CompoundName;
class Relation;
class OptimizationOptions;
class ExecutionUnit;
class Receiver;
class Table;
class DataStore;
class Optimizer;
class Columns;
class PredicateName;
struct SourceLocation;
struct ParseData;

typedef std::shared_ptr<Evaluation> EvaluationPtr;
typedef Entity * Row;

typedef int EntityId;
typedef int StringId;
typedef int Arity;
typedef int RelationId;
typedef int VariableId;

typedef std::size_t Size;
typedef std::size_t RowIndex;

namespace Logical
{
    class Call;
    class Module;
    typedef void(*Extern)(Call&);
}

namespace persist
{
    class shared_memory;
}
