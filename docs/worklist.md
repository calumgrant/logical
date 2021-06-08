# Work plan

- `new5` failure. `DeduplicateV` needs to share the table in the rule somehow.

- Special test, `-fclone` which simply clones all rules

- Define the bound predicate: problem is
  1. We don't know which variables are used in the outputs
  2. With multiple branches, the output variables could be duplicated.
  3. Output variables are sometimes computed.

- Implement a variable checker
  - No undefined reads.
  - No duplicate writes.
  - No dead writes.

- Remove some dead classes in EvaluationImpl
- What about AddBBB ?? Implement as:
  c = a+b, c=d, 
- Check negate FB (Same as negateBF)


Solution: 
1. Ensure a consistent set of variables are used for the inputs/outputs. Make them variables 0-n.
   - Need to remap all of the variables so they are consistent? But how?
   - For all writes, identify which variables are used, then remap all variables in the evaluation so that the
     written variables are at the front.
2. Insert an extra call at the head of each rule in the predicate.
3. Call `Rebind(boundset)` on each evaluation node to get the correct binding.

Step::RemapVariables(const std::vector<int> & map)
std::vector<int> variableMap



- Have an "abstract machine" calling convention, where a "ret" on the stack jumps to the address
  - stack pointer
  - stack of values (entities)
  - 

- new objects to define a variable, and use `and` to assert further facts, for example
`new expression p has ...,p has parent e.`

# Datalog abstract machine

State:
- A stack of values
- A stack of continue/return pointers. Previous stack pointer (on return)
- instruction pointer
- stack pointer

- Instructions:
  Load immediate value.
  Assign, Check equal
  Call: target address, stack size
  Continue
  Return




- Call the bound predicate

Creating a bound predicate: Problems if there are multiple branches in a rule
- Don't know which variables to bind on input as it's different per branch.
- Morally it's just before the write, but we need to reorder it.

Semi-naive evaluation:
1. Implement BindingAnalysis, that marks columns that are always bound.
2. Implement CallBoundPredicates, which creates a separate relation for the bound columns, and calls that.
3. Implement a BoundPredicate, which must be called with the given fields set.
  a. Implement a "Load argument" evaluation step that loads values from the query. Basically a delta request. `delta-semi-naive (_0,_1)` -> 
  b. 
  b. Implement a "Write result" evaluation that writes values to caller or intermediate table.
  c. Implement a cache of results: 1. A table of inputs that have already been computed, and 2. The regular table of outputs.

Recursive bound predicates: When a recursive bound-predicate is called, the arguments are added to the table of arguments, and they are evaluated the next iteration.

q1(X) :- descendent1(5, X).
q2(X) :- descendent2(5, X).
descendent(X,Y) :- child (X,Y); child(X,Z), descendent(Z,Y).
descendent2(X,Y) :- child (X,Y); descendent(X,Z), child(Z,Y).

This notices that descendent is always bound in the first argument. (Hard as it's conditional). The rules become:

descendent1(X,Y) :- args1(X), (child (X,Y); child(X,Z), descendent(Z,Y)).
descendent2(X,Y) :- args2(X), (child(X,Y); descendent2(X,Z), child(Z,Y)).
args2(5). // From q2
args1(5). // From q1
args1(Z) :- args1(X), child(X,Z).

What about things that aren't recursive?

The beauty of this idea is that it does not need changes to the rule engine.

Implementation plan:
1. Implement bindinganalysis
2. Create "binding" predicates: `bind_bff:descendent`
3. For all calls, insert an additional "write" to the binding predicates.
4. Add rules to the binding predicates.
5. For relevent calls, call the bound predicate instead. `has_bf:descendent`
6. Compile rules for the bound predicate, inserting a call to `args_n:` at the head, and rebinding all steps.
    `std::shared_ptr<Evaluation> Evaluation::Bind(std::vector<int> arguments_to_bind)`


2. Change call sites to also write


Need to remember that we need to continue evaluating the recursive predicate.

How to implement a bound predicate:
1. C

2. Those queries that are always bound are compiled with their bound variables.


- Implement a CallAnalysis, that collects information on bound variables.
  - flag: Will enumerate all
  - mask: boundfields: Fields that are bound for every query.
  - Perform a whole-program analysis maybe?
  - Problem: How does this interact with the join orderer?

- Works top-down:

- Avoid reanalysis of shared steps in queries.

- Show binding errors for `has:strlen` etc

- Optimization: delete tables when finished.
- Garbage collection: 

Newtypes:
- Performance bug in `performance/new3.dl` - too much memory used.

Implement semi-naive recursion:
- Implement the new optimization SemiNaive
- Determine how predicates are called
- Flag certain predicates as semi-naive.
- Determine whether the predicate is recursive

- Analysis: Query types. For each relation, store a set of how the relation would be indexed, ahead of time.
- After the rules are run, clean them up to save memory.
- Optimization: Merge heads.
- Optimization: Parallelize.

- Predicate: clear "all". To run multiple tests in the same run.

- Optimization: Compressed table type. Only for streaming. Use deltas or something.

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

# Strings
List of predicates:
  - `has:length`
  - `has:substring:from:to`
  - `has:lowercase`, `has:uppercase`
  - `has:regex-match`
  - `has:char:at`

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

## Semi-naive evaluation

- Use square brackets for special annotations, such as: `[in]`, `[out]`
- Syntax highlighter

- Optimization: Avoid redundant writes.

- Bug `number X has square Y if number X and Y = X*X.` is not really recursive.

- Problem is rules attached to multiple predicates. How does the analysis work there???

- Unit tests for tables.
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

## Short term

Problems to solve
- Defining data like `temperature -5.`
- Reporting the line number of negative recursion.

- Use termcap library

For each predicate:
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

6. Implement some of the optimizations
  - Unused variables.
  - Mark certain predicates as deltas.

- Put information into each evaluation step:

- Create a `VariableInfo` structure
  - slot
  - bound
  - last use?
  - With pretty printing?
- Detect last use and use an "exists" predicate as needed.
  - `bool Relation::Exists(const char * Entity, int mask)`
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
- Syntax errors - Count in the error count.
- Debug steps: expect... to expect a certain number of results, or a certain number of evaluation steps.
  - Parser error recovery with `.`
- Embedding Logical
- Extending Logical
- Perhaps have an `Evaluation::SetRow()` so that it's possible to store the row?
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

## Recursion

- Error on negative recursion
 - How to even detect it
 - Have a flag called 'Evaluating', as well as a parity.
 - Probably want some kind of static analysis to be honest.
 - When can we use the delta??
- Can we avoid unnecessary branches in the recursive step??
- Can we just use the deltas??

## Semi-naive evaluation

- Can we use context when compiling a rule?
  - Yes! When we call eval, we know which variables are already bound

# Unresolved issues
- Putting `-` into identifiers?
- Semi-naive evaluation where some of the variables are already bound.
- Recursion. Needs to work with semi-naive evaluation.

## Datalog predicates

- nary predicates
- Perhaps mix Datalog syntax with Logical syntax.
- Syntax for all, count etc.

## Last use optimization

## Recursion

## Memory management

Implement memory mapped memory allocator.

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
- [ ] MySQL connector. Implemented using DLLs?
- [ ] Dynamic-linked libraries
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
