# Work plan

```
  s has lowercase "fubar".
  Str has character 'c', position 12.
  str has substring "abc", position 12, length 3.

  // Now this predicate is also unbound.
  str is a short string if str has strlen n and n<5.
```
The new predicate names are: `has:length` and `has:character:position`. These are simply builtin predicates. Need to avoid creating the projections to and from these predicates. They also have special binding rules that we need to consider. We have a non-binding column in general.
  - `int Relation::Bind(int columns)`
  - Report errors if binding is impossible.

What happens if you try to add data to a builtin predicate??? Is this the same as a rule/semi-naive evaluation???
  has:length:3

How does `has:character:position` map to `has:character`? They need to be separate predicates.

```

```




- Optimization: delete tables when finished.
- Garbage collection: 

Newtypes:
- Performance bug in `performance/new3.dl` - too much memory used.

Implement semi-naive recursion:
- Implement the new optimization SemiNaive
- Determine how predicates are called
- Flag certain predicates as semi-naive.
- Determine whether the predicate is recursive

String length: 

- Analysis: Query types. For each relation, store a set of how the relation would be indexed, ahead of time.
- After the rules are run, clean them up to save memory.
- Optimization: Merge heads.
- Optimization: Parallelize.

- Predicate: clear "all". To run multiple tests at the same time?

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

# Newtypes or record types

```
new person has name "Fred", age 20.
```

The idea of `new` is to create new entities.

```
new parameter has name n, index i, parent f if 
  function f has parameter n, index i.
```

Implementation: There is a new entity type called `newtype` that 

predicate `has:name:index:parent`

Binary format of newtypes? Not stored.


# Strings
List of predicates:
  - `has:length`
  - `has:substring:from:to`
  - `has:lowercase`, `has:uppercase`
  - `has:regex-match`
  - `has:char:at`


Work plan:
- Builtin predicates for attributes:
  - string s has length 12.
  - string s has length=12
  - `string s has length < 12`
- What happens when you define a builtin predicate???
  `@1 has length 1.` then we query what strings have length??
  `_ has length 5.`
  With no types, we have


  - substring
  - find, indexof.
  - matches
  - lowercase `name has lowercase "foo"`

`param has substring p and p has lowercase "pass"`
`param has lowercase p and p has substring "pass"`

"foo" has character "f", position 0.
"foo" has substring "fo", position 0, index 0.
"f" has match "foofoo", position x and 

- string/regex match
  `REGEX has regex-match Result`
  `X has equal Y.`


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
- When counting and summing, ensure we make the body reentrant; deduplicate isn't doing enough and needs to be reset.
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
- Bound variables (reads)
- Unbound variables (writes)
- Size estimates
- Read relations
- Written relations
- Successors

- Create a `VariableInfo` structure
  - slot
  - bound
  - last use?
  - With pretty printing?
- Detect last use and use an "exists" predicate as needed.
  - `bool Relation::Exists(const char * Entity, int mask)`
- `order by` as a database concept...

- `expected-errors` to count the number of errors.
- Closures:
  - `call reaches a Successor succ`
  - `x reaches a parent y`
  - Think about + vs * keyword
  - Implementation? 

- Time each predicate.
- Better results
 - Sort the results
 - Deduplicate the results
  - Expect a certain number of results. If not, report an error.
     `expect 100.`
  - Implement `find X,Y,Z in`.
- Column names in `find` and `query`.
- Implement a resultset. It's just a table with a writer. The table stores the results.
- Consider synthesising an ID for `has:Person:Name`
- Log filename in errors.

## Recursion analysis

1. Mark the body of each "not" with a flag. This flips the parity bit during the depth-first search.
2. Start at the "query" and perform a search through all evaluation steps, marking each step as "visited". When you reach a step that's already visited, mark it as "recursive". Then on the way back through the depth-first search, mark those steps as "recursive loop" as well.
3. Predicates that are flagged as recursive.

- What if something is recursive in one evaluation but not another?? Need to do it on a per-predicate basis.

## Refactoring
- Implement a `ChainedEvaluation` class that exposes `Next`?
- Remove the AST `Visit` functions
- const fields more
- Split up files
- Put into `namespace Logical`
- Slim down the header files, for example split up relation.hpp

- Bug: We find an empty rule when attempting to add new rules to an existing query, for example when creating a projection.

- Colour-code the output explanations.
  - Finish all of the evaluation types
  - Refactor this properly
  - Colouration options: Use termcap, and work on Windows.
- Warning empty relation:
  - Create a line number
  - Make it red.
- Syntax errors - Count in the error count.
- `expect` predicate - number of evaluation steps. Prints a message
- `steps` predicate - gets the current number of evaluation steps
  `if S is steps then print "Currently at " + S + " steps".`
  `print "..." if steps S.`
- Debug steps: expect... to expect a certain number of results, or a certain number of evaluation steps.
  - Parser error recovery with `.`
- Embedding Logical
- Extending Logical
- Optimization passes
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
- Think about predicate-names containing `-`.
  - Problem is negative facts like `print -2.` Try to turn it into an entity.
  - `error "negative" if not 5 has negative -5.` does not work.
  - Expressions like `foo X+1` doesn't parse yet, as it could be `foo (X)` which then becomes a Datalog predicate....
- Binding issues
  - `number X has square X*X.` is not bound yet.
  - `number X has square Y if Y = X*X.` also not bound yet.
  - Binding tests
- Missing input files are not displayed as errors.
- `is not` syntax.
  - `X is not a person`.
  - `X has no job`
- `not` should fail early (optimization) - see the primes1.dl example
- `or` join both branches if the same variables are bound in all branches.
- Ressurect the ramp and persist projects. Probably persist mainly.
- The indent should be 1, not 4.
- Nicer closure syntax.
- Installer
- Build and test on github actions
- Optimization: Tables should assume a single type, then fall back onto polymorphic behaviour which is slower.
- Free intermediate tables, for example tables used for deduplicating.
- Or just delete the rules?
- Is the deduplicating logic even sound?? Surely other variables can change too? So we need to deduplicate lots of variables, not just the ones in the sum.
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
