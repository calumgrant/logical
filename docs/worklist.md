Next: antlr parsing
Next: Externs that set data.

Bug: csv:read is evaluated twice in this:
```
if
    testcase F has contents C
then
    file F has contents C and csv:read F.
```

# Ideas to work on next:

1. Big code tidy
  - Split up files
  - Tests working on Actions
  - Work on code spaces
2. Release tasks
  - Documentation
  - Test on Linux
  - Test on Windows
  - Squash history
  - Command line options and help text.
  - Enable specific optimizations
3. External API
  - Reference predicates
    - Write to them (assert)
    - Bulk write
    - Query them
4. Modules
  - CSV files
  - MySQL 
5. Schedule execution
  - Each execution unit has prerequisites
6. Better error messages
  - Expected number of errors
  - Better inserts
  - Locations on all nodes.
  - Performance diagnostics (list expensive predicates)

# Future plans

1. Compilation to C
2. Parallelism
3. ANTLR parsers
4. Optimizations

- Check partial evaluation with both columns bound.
- Check that the evaluation 
- Tidy up help text when Logical started.

- [ ] report of predicate performance (by number of steps).

# External compilation of arbitrary calls

We know how to enumerate a table, but what about a call in general?

```
class MyCall : public Call
{
    void First()
    {
        // Reset all counters
    }

    bool Next(Int & v1, Int &v2)
    {
        // Produce one more result
    }
};

class MyCounter : public Call
{
  Int a, b;

  void First(Int a, Int b)
  {
    this->a = a;
    this->b = b;
  }

  bool Next(Int & result)
  {
    if(a<=b)
    {
      result = a++;
      return true;
    }
    return false;
  }
};

class PredicateCall : public Call
{
  Index i;
  Enumerator e;

  void First()
  {
  }

  template<typename...Ints>
  bool Next(Ints&&... x)
  {
    return i.Next(e, arity, x...);
  }
};

class ComputedPredicateCall : public Call
{
  void Init(Row data);
  void Query();
  bool Next();
};
```

```
csv:filename name has row x, column y, text x.

csv:filename name has row r, surname _, forename _. 
```

Idea: Inline predicates. Make the projections inline. Have some rules as being marked "inline" and can be partially bound. This could avoid storing intermediate results.

Allow unnamed attributes, e.g. `mysql:query "..." has x, y, z.`

Enumerating arbitrary predicates
- As if they were inline
- How can we use an enumerator???

# Implementing a bytecode machine (LLVM-lite) -- future

Metadata:
- Predicate name (Int id)
- Dependencies predicates (list) (Int id)
- Number of stack variables
- Number of tables

Enumerators use one stack variable (putting both `i` and `j` into one 64-bit int).
Predicates are identified by a string (e.g. `string:length`)
There can be up to 256 tables and variables
There can be up to 256 jump positions
Can we use native types for arithmetic etc if we know them?

Instructions to support:
- `create table arity`
- `query table enumerator registers...` start a query
- `read branch table enumerator registers...` read from a table
- `return table` stop evaluation of predicate
- `beq v1 v2 branch` branch if equal
- `bneq`, `blt` etc
- `bnone v` branch if v is none
- `load v c` Load a constant into a variable
- `copy v1 v2` Copy a value from one variable to another
- `inc v` Increment a value
- `write table arity variables...` Write a table
- `yield arity variables...` Yield a result from the predicate ??
- `goto branch` Jump to target location
- `add`, `sub` etc (arithmetic instructions)

E.g. iterate over a table

  query table, enumerator, arity, variable...
  read table, enumerator, arity, variable..., branch
  write table, arity, variable...

Encode externals as:

External API example
  i.find(e, binding, in(x), out(), z )

```
item:
  locals L0, L1, L2, ....
  table T1, 3
  ld L0, 1
  ld L1, 10
  cp L0, L2
loop:
  bgt L2, L1 end:
  ld L3, @1
  ld L6, 
  write T1, l3, l5, l7
  sub L7, L2, L6
  goto loop:
end:
  return T1

error:
  depends t1, item:
  locals l0, l1, l2, e1
  table result, 1
  ld l0, true
  ld l2, @1
  query t1, e1, l2
  read end: t1, te
  ld l1, "item"
  write result, l1
end:
  return result
```

What about enumerators for computed predicates?

```
class CodeGen
{
  // Creates a new label
  L CreateLabel();

  // Label current instruction as a jump target
  void Label(L);

  void Goto(L);
  T CreateTable(arity);
  V CreateVariable();

  void Copy(V, V);
  void Load(V, int);
  void Load(V, string);

  void Add(V, V, V);
  void Blt(B, V, V);

  void Query(T, E, V...);
  void Read(L, T, E, V...);

  void Return(T);

  void NewPredicate(PredicateName);
  T Dependency(PredicateName);
};
```

# Plan for next week
- [X] Finish implementing tables for the external API
- [X] Finish external API
- [ ] Finish SQL connector
- [ ] Finish semi-naive evaluation

## Tables
  - Compact tables
    - Stores values as `std::int32_t` and has a set of columns (EntityType)
    - All entitytypes can support `Entity(t, v)` which converts an entity from short form to long form.
  - [X] All enumerators consist of a class `Enumerator` containing 2 32-bit ints.
- Finalise table after evaluation -> turn it into a `SortedTable<>`

## Finish the external API
- [X] Fix naming scheme, e.g. `mysql:database db has username foo`. `mysql:Test:person id has name name`
- [X] Refactor `CompoundName`
- [X] Variadic externs
- [ ] External API supports queries
  - `ExternalSortedTable<> call.GetResults(index)`
  - `call.SetResults(...)`
  - `module.AddPredicate(name, dependencies)`
- [ ] Computed predicates - cached.
- [ ] External predicates as tables not predicates.

## Finish MySQL
- [ ] Queries with inserts, e.g. `mysql:query "SELECT surname {1} FROM Person WHERE id={0}" has name "Frank", surname S.
  `mysql:query "INSERT ({0},{1}) INTO Person" has column1 X, column2 Y if person X has name Y.`
- [ ] Queries on bound columns, e.g. `mysql:Test:Person 123 has surname X`
- [ ] What about projections??
- [ ] Connection pool

# General ideas

- Optimization to discard items that won't be used again?
- Compilation hints
  - `[keep]` Don't unload the results
  - `[noreorder]` Don't reorder evaluation
  - `[inline]` Make inline wherever possible.
  - `[...]` Consistent with opimization options wherever possible.
  - `[..., ...]` A list of options.
  - `[first]`
  - `[last]` perform something last
  - `[immediate]` evaluate this immediately.

- Think more about compilation scheme
  - Control flow for count, sum and not.
  - Deduplication
  - FIFOs for recursion.
  - Semi-naive evaluation and recursion.
  - Microthreads

- Optimization: Can "push" all results to the first tuple. Can auto-inline any predicate that is never queried. We can "push" all results to other predicates without storing them.

```
struct Task
{
  std::atomic<Task *> next;
  virtual void Run()=0;
};
```

- Split recursive predicates up into two loops (base case and recursive).

- Optimization: Drop deduplication guard on things that are already deduplicated.

- Message of the day.

- Fail to compile a clause on binding error. This actually crashes (`binding.dl`).

- Externs
  - Check binding errors at compile time?
  - Think about whether we want to change the `Table` not the predicate?
- All errors to have locations
- Locations to have filenames
- Test projecting external predicates.
- Private symbols in modules?

## Running as a server

(As an alternative to using "persist"). TODO: Get "Persist" working.

The `-S` option creates a new server, killing the existing server if required. The `-s` option connects to an existing server.

You can specify the name of the socket, for example `-ssocket1` which will create a socket `socket1` in the local directory. If no name is specified, then Logical will use the name `logical.socket` in the current directory.

## Task manager

Lightweight thread manager. Could just use C++ taskflow library instead?

```
class Task
{
  std::atomic<Task*> list;

  enum State { Inactive, Queued, Running, Finished } state;
  virtual void Run(TaskManager & manager);
  Task & next;
};

class Foo : public Task
{

};

class TaskManager
{
public:
  void Run(Task[] tasks, int length, Task & then);
};

class CompositeTask : public Task
{

};
```

Expressing execution as a graph.

## General ideas

- Optimization: A regex pool to speed up regex search.
- Compile to a proper bytecode that can be stored in the datafile.

- Trap importer extern.
```
load-module codeql.

codeql:import-trap().

function f if codeql:trap:functions f
```

- Memoised externs. E.g. from a trap-import.
- Allow commands of the form `codeql:import-trap` - nonary predicate.

- Avoid evaluating rules twice.
- Optimization step: Figure out if rules already run.

- Store the original source line for each rule, and highlight the parts of it as it executes (in Explain)
```
    Evaluate with 4 variables (called 1 time) ->
        Scan person (_) -> (_0) (called 1 time) ->
            Load _2 := @f (called 3 times) ->
                Write (_0,_2) into has:name:gender:b_ (called 3 times)
                Join has:name:gender:B_B (_0,_,_2) -> (_,_1,_) (called 3 times) ->
                    Deduplicate (_1) (called 2 times) ->
                        Create new _3 (called 0 times) ->
                            Write (_3) into woman (called 0 times)
                            Write (_3,_1) into has:name:B_ (called 0 times)
                            
```
becomes (* = write, _ = read)
```
    new woman has name n if person *_* has name n, gender @f.
    new woman has name n if person _ has name n, gender *@f*.
    new woman has name n if person _ has *name* n, *gender* @f.
    *new woman* has name n if person _ has name n, gender @f.
    new woman has *name* n if person _ has name n, gender @f.
```

- Avoid virtual functions for efficiency, for example in `Database` and `Relation`.

- What about AddBBB ?? Implement as:
  c = a+b, c=d, 
- Check negate FB (Same as negateBF)

- new objects to define a variable, and use `and` to assert further facts, for example
  `new expression p has ...,p has parent e.`

# Parallelism

Options for parallelism:
1) Evaluate the ExecutionUnits in parallel.
  - Implement the partial order
  - Put a lock on each table
2) Implement each ExecutionUnit in parallel. This should give the benefit when we are stuck on one or two big predicates which resource-starve the CPU.
  - Split the execution frame into N different branches.
  - The outermost/initial step is the one to parallelise.
  - Implement a "QueryParallel" execution node
  - Implement a "WriteConcurrent" execution node.
  - Table->CreateQuery(Row row, Columns)
    - Table::WholeTableIterator
    - Table::ColumnsIterator
    - Table::ReentrantIterator
    - What about other table representations? (e.g. a sorted list)
    - This can only happen once the Table has been defined.
    - Could we just have a single cursor in the table??? No, anti-parallel.
```
class Task
{
  virtual void Run() =0;
};

class EvalLoop : public Task
{
  std::vector<Entity> locals;
  void Run() override
  {
    while(query.GetNext(locals.data()))
    {
      next->Evaluate(row);
    }
  }
};
```

Threading engine: An engine contains a threadpool, and you add new threads using `Database::RunTasks(Tasks[], int size)`. Generally we want more threads than the level of CPU parallelism. This will run all of the tasks to the required level of concurrency, then return. The main tasks are:

1. Running a parallel query, particularly an unbound scan or the scan of a delta.
2. Running execution units in parallel when it is safe to do so.

Changes to make:
1. Each table is threadsafe. What this means: Concurrent writes are possible and are safe. Use atomic counters in the execution unit. Reads are also threadsafe, and can overlap with writes. (May need to copy row data to avoid vector moving on write). 
2. The persist memory allocator is threadsafe. Use atomic lists for the memory pool. Use atomic counters.
3. Threading options: -t1, -t -t0 (infinite), -tn. Default for -O0 is -t1, and -
4. The first read of an evaluation:
   - Keeps a vector of parallel stack frames as single array
   - Keeps a vector of Tasks
   - OnRow: Performs the query, returning a list of rows. Dispatch the first n results as threads. Each thread: runs the next function, then when the function returns, gets the next row in the list. Keeps a "count" of active threads, and when the count goes to 0, OnRow can return.

## Changes to "Persist"

- Need to be able to extend the heap in a thread-safe manner.
- mremap?
- Need proper multi-threaded soak tests
- Use atomics for the memory allocator

- How to implement a bound predicate:
2. Those queries that are always bound are compiled with their bound variables.


- Avoid reanalysis of shared steps in queries.

- Show binding errors for `has:strlen` etc

- Optimization: delete tables when finished.
- Garbage collection: 

- Performance bug in `performance/new3.dl` - too much memory used.

- Analysis: Query types. For each relation, store a set of how the relation would be indexed, ahead of time.
- After the rules are run, clean them up to save memory.
- Optimization: Merge heads.
- Optimization: Parallelize.

- Predicate: clear all. To run multiple tests in the same run.

- Optimization: Compressed table type. Only for streaming. Use deltas or something.

# Debugging

## Enabling the debugger

The debugger is enabled by passing in the `-d` option to the command line, or by asserting `debug true.`. The `-i` option also displays a prompt.

The debugging prompt accepts regular Logical syntax.

## Using the debugger

Commands are:

- `r` Continue running.
- `q` Quit debugging and exit.
- `k` Stop debugging.
- `s` Step in
- `n` Step next
- `o` Step out
- `h` Display help
- Logical line: Interpret the current line as Logical syntax.

The empty line runs the previous command.

Each step displays:
- The values in the bound local variables
- The values in the bound program variables
- The current source line, highlighting the section that's being executed.
- The current evaluation step

# Structured values

There is no reason why we can't have have a "row" as a data item. What does this mean?

```
number 0.
number S if number N and S = {succ N}.
number {succ N} if number N.
```
This creates a predicate `number/1` and `succ/1`.
```
parameter {F has parameter P, name N, index I} if ...
```

Can it refer to itself? Sure! But be sure to end the recursion.

```
number 0.
number {number N} if ...

node {function F} if function F.
node N if N = {function F}

node {function F} has name Name if 
  F has name Name.

node {call c} has type type if
  c has target t and
  t has return-type type.
```

## Specification

An entity-expression of the form `{ ... }` defines a "structured value" representing rows in a predicate. For example `X = { F is a function }` represents a row holding `F`, rather than `F` itself.  Since a row cannot refer to itself, the clause `F = { F is a function }` always fails.

Structured values offer another way to extend the data.

Structured values may be nested.

Structured values can be recursive, but there is a danger of infinite recursion

## Example: Lists.
```
list @empty.
{H has cons T} has head H, tail T.
```

## Implementation

Storing this would be as simple as having a "row" type consisting of a relation-id (16 bits) and an index (32-bits). 

Strings could be just another table, as could "at-strings".

Relation::Query() could have an analog, Relation::QueryRowIds(), which returns a single value for each result, encoding the row-id.

The evaluation steps are:

```
Unpack _5 -> (_6, _7)
```

This takes an existing row-id, looks up its table, and gets the row data.

``` 
Query relation (_6, _7) -> _5
```

This performs a join (scan, probe, join) and returns the results as a row-id.

The database contains a map from relation-id to relation. This is only done for relations that are actively queried, since there are only 16 bits used to store the relation-id.

Efficiency: `person { node @123 }` compiles to `_0 = @123, probe node (_0) -> _1, exists person (_1) -> _`.
`person { node N }` where `N` is unbound, compiles to
  `scan node (_) -> _0 = (_1), probe person (_0)`
it could also compile to:
  `scan person (_) -> (_0), probe node _0, unpack node (_1) = _0`

There is a slight danger of inefficiency here, compared with the `new` keyword.

What about transitive closures?

```
person { X has name Y }.

person { @1 has name Y}.
Gets evaluated as
_0 = @1
find has:name (_0,_) -> _2 = (_,_1)
  exists person _2
```
so you can't unpick it to the same extend as `new`.

## Displaying structured values

Structured values are displayed in an unpacked form. We don't want to expose internal IDs. For example

```
person @1
person (@1 has name "Fred")
number (succ 0).

new number has value 0.
new number has value n+1 if number _ has value n and n<10.

vs.

number (value 0).
number (value n+1) if number(value n) and n<10.
number V if number N and N = value N0 and V = value

value 0
value 1
...
value 10
```

# Built-in predicates

- String predicates (See list below).
- Query predicate `query`
- Error `error`
- Print `print`
- Exit `exit`
- Evaluation
  - `expected-results`
  - `evaluation-step-limit`
  - Memory limit get/set `evaluation-memory-limit`
  - Time limit get/set `evaluation-timeout`
  - Optimization options
    - optimization-level get/set `optimization-level`
- Modules for builtins?
  `logical:length` for string length??

- Query optimization rules
```
if optimization-level is 0 then step-limit is 1000.
```

- Optimization: Don't evaluate a rule twice if it's not recursive. (e.g. when attached to multiple predicates.)
- Report error when defining a builtin predicate.

# Persist

- Bug in UnaryPredicate::Assert. We look up the unary relation, which returns something that is invalid. The ->Add() predicate fails and crashes. Is it due to invalid function pointers in vtables?
  - Check resource limit
  - readme
  - proper tests
  - Tidy up the code generally
  - Robustness to invalid input parameters and proper diagnostics
- Tidy up tests
- Allocate a huge file but don't allocate it all on disk?
- Create a decent test framework support
- More robust open file options
- Errors if size is invalid
- Throw on failed
- Easier to create a temp file.
- Set the size limit.

- Report number of unique tuples written in a write predicate

- Reset counters properly for deduplications (sum/count)
    `class DuplicateScope`

# Simpletest
- Give it a repo
- Name it something better
- Simpletest to count fixtures?
- Test exceptions
- Compare vectors
- Tidy up code

2. Implement `-f` and `-fno-` to enable and disable options.

- Use square brackets for special annotations, such as: `[in]`, `[out]`
- Syntax highlighter

- Help option: `-h`
- `logical:option "no-joinreorder"`.
- Have a better find syntax.
- A question at the end of a clause turns it into a query?
```
find x, y in x reaches child y.

```
- Any predicate can be a query.
- `null` as a keyword is simply the null value. (Not the same as "none"??)

```
result X has foo Y if ...

find result, foo, bar.
```

- Inlining predicates
  - When?
  - If only used once.
  - If deemed to be very big
  - If recursive
  - How?
    1. Allocate a block of variables: one for each branch.
    2. Clone the evaluation, removing all other writes, and also the final write.
    3. Replace the final write(s) with the tail.

- More succint explain

```
    _0 := 1
    _1 := 10
    For _0 <= 2 <= _1:
        _4 := _2 + _3
        Write has:next(_2,_4)
    For (_0,_1) in has:next(_,_):
        Write has:next2(_0,_1)
    For (_0,_1) in has_next3:
        2 := "Mutual recursion"
        Write query(_2)
        Write has:Number:Next(_2,_0,_1)
    For fixed-point (_0,_1):
        For (_0,_1) in has:next2:
            For (_2,_) in delta_has_next2(_,_1):
              Write has:next2(_0,_1)
```

# Files API

read-file "filename".
```
// Cached
file "filename" has line l, text t.

file f has line l, section s if
  file f has line l, text s
  and
  t has regex-match s

file f has line l, key k, value v if
```

## Short term

- Reporting the line number of negative recursion.
- Use termcap library
- Successful evaluation message in bright green.
- Join order: Put the recursive delta first (special case of the join orderer)
- Optimization: Or: merge identical steps (can do this after local variable optimization maybe).
- Merge identical predicates (optimization)
- Idea: Have an optimization threshold. If two sizes are equivalent to within a factor of N, then don't reorder them.
So the existing evaluation order is taken as a compiler hint.
- Idea: Have options controlling optimization

```
logical:limit has steps 100.
option @evaluation, steps 100, timeout 1000.
option @results, expect 1000.
logical:reorder false.
// option @optimizer, reorder false, level 2.
```

- non-ary predicates
foo if bar and baz.

- Put information into each evaluation step:

- Create a `VariableInfo` structure
  - slot
  - bound
  - last use?
  - With pretty printing?
- `order by` as a database concept...

- Better results
 - Sort the results
 - Deduplicate the results
  - Implement `find X,Y,Z in`.
- Column names in `find` and `query`.
- Implement a resultset. It's just a table with a writer. The table stores the results.
- Log filename in errors.

## Refactoring
- Remove the AST `Visit` functions
- const fields more
- Split up files
- Put into `namespace Logical`

- Bug: We find an empty rule when attempting to add new rules to an existing query, for example when creating a projection.

- Colour-code the output explanations.
  - Finish all of the evaluation types
  - Refactor this properly
  - Colouration options: Use termcap, and work on Windows.
- Warning empty relation:
  - Create a line number
  - Make it red.
- Parser error recovery with `.`
- Embedding Logical
- Extending Logical
- Report duplicate attributes a bit better.
- Count total number of rows stored.
- Count total number of predicates.
- Warning on undefined predicates.
- File operations
  - `Database::ReadBinary()`
  - `Database::WriteBinary()`
- C-style multiline comments
- Implement Datalog syntax
  - unary and binary relations
  - all and count syntax
  - tests for these
  - n-ary relations
    - n-ary tables
    - efficient join orders.
  - how to name arbitrary n-ary tuples from Datalog that are compatible with logical? Use `:` to access them.
- Inefficient to run `if 1<=X<=100 then X is c and X is b.` as it's evaluated twice.
- `exit`
- Binding issues
  - Binding tests
- Missing input files are not displayed as errors.
- `is not` syntax.
  - `X is not a person`.
  - `X has no job`
- `not` should fail early (optimization) - see the primes1.dl example
- `or` join both branches if the same variables are bound in all branches.
- The indent should be 1, not 4.
- Installer
- Build and test on github actions
- Optimization: Tables should assume a single type, then fall back onto polymorphic behaviour which is slower.
- Free intermediate tables, for example tables used for deduplicating.
  - Or just delete the rules?
- Problem with adding rules on demand if a predicate is already being evaluated.
- Implement efficient indexing of inequality: `p has Age a and a>10`
- Putting `-` into identifiers?

## Datalog predicates

- nary predicates
- Perhaps mix Datalog syntax with Logical syntax.
- Syntax for all, count etc.

# Project ideas

- [ ] Optimizer. In particular, the join orderer
- [ ] Magic sets optimization.
- [ ] Incremental updates, such as reevaluating predicates if data added or removed. Incremental evaluation in general. Making data dirty.
- [ ] Persistence layer.
- [ ] Module system.
- [ ] JIT compiler - take into consideration the sizes of things before committing to a join order. Evaluate the dependencies first, then compile.
- [ ] Better test support - compare two predicates.
- [ ] Threading support
- [ ] Client-server implementation.
- [ ] Interactive debugger.
- [ ] Interactive shell.
- [ ] Command line option to pass code
- [ ] A compiled bytecode for Datalog.
- [ ] Resources limits - memory, time and tuple-counts.
- [ ] Reading external data.

# Optimizer
- JIT optimizer - create the basic predicate, then optimize based on table sizes.
- Avoid duplicate loads. If two constants share the same value on the same path then make one an alias for the other.
- Lift constants out of loops.
- Optimize variable layout, for example reuse local variables 
- Avoid duplicate tests, if a condition is always true (or false).
- Last use optimization - detect when a variable is not used and remove it. Turn joins into exists.
- Join identical paths
- Remove "NoneEvaluations"
- Push context
- Join orderer, based on sizes of tables.
- Avoid reevaluation of base case in recursion.
- Inline predicates sometimes.
- Use datatypes, for example if a variable has just one type then the calculation could be faster.
- Lay out the locals to make querying more efficient.

# Release plan

## Initial release

The initial release will be a simple in-memory Datalog interpreter, supporting all basic Datalog features, including count, all, sum, not, arithmetic, recursive evaluation. It will not include an optimizer, or offer persistent storage.

 - [ ] Readme finished
 - [ ] Tutorial finished
 - [ ] Contributing guides finished
 - [ ] License
 - [ ] Open source the repo
 - [ ] Performance tests
 - [ ] Compilation on multiple platforms
 - [ ] Create issues for contributors to work on.
 - [ ] Imports
 - [ ] Command line arguments: Inputs from stdin, output to file.

## Second release

# Documentation

- [ ] Readme
- [ ] Tutorial
- [ ] Manual
- [ ] Contributing guide, how to build
