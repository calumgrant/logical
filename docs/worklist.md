# Work plan

## Short term

Problems to solve
- Ensuring that recursion remains correct
- Use the delta when evaluating recursion
- Have a range of optimization options - so we can validate the changes.
- Avoid reevaluating branches unnecessarily, in recursion and when rules attach to multiple tables.
- Defining data like `temperature -5.`
- Detecting negative recursion.

Work plan:
4. Hook in an analysis framework.
5. Detect negative recursion.
6. Implement some of the optimizations
  - Unused variables.
  - Mark certain predicates as deltas.

Implementation of recursion:
When a query (scan/join/probe etc) is made on a predicate that's marked as "evaluating", then the *query* is marked as recursive, and the current branch/rule is also marked as recursive. (How?) Problem: what if it becomes recursive?
The query that first marks itself as recursive may join on the delta, bec

Idea number 2: Perform a static analysis at time of evaluation.
Implement as depth-first search where if you meet something that has been found before then you mark it in a recursive loop. This step also checks for parity.

How to detect "not" and other negativity in rules?

Design: Every time a predicate is evaluated, if needs to check the evaluation to see if it has been analysed. The analysis will perform a number of steps:

1. Walks the dependency graph, checking for recursion and negative recursion. Recursive predicates and queries are marked as such. Negative recursion results in an error. Marks the first predicate as a delta evaluation.
2. 

- Put information into each evaluation step:
- Bound variables (reads)
- Unbound variables (writes)
- Size estimates
- Read relations
- Written relations
- Successors

class Evaluation::Visitor

Evaluation::Visit
int number of BoundVariables


Mutual recursion?



1. Don't reevaluate non-recursive branches.
2. We only need to query/join the delta.

If, when we run a rule in a predicate, it doesn't *query* anything recursively, then there's no need to re-run that branch, and we can tag that branch as "done".

Detect recursion on path: Run Query -> Write data. At end of
1. Evaluate predicate
2. Run query on predicate. Flag the "current branch" as recursive. Flag the *first* query as recursive. The *second* query is not recursive.


- In `closure1.dl`, we end up calling `has:descendant` 37952 times.
  - We need to join with the delta `has:descendant∆`
  - We need to tag the evaluation that could be recursive.
  - How do we know?????
  - When exiting a join, we may discover that the join was recursive. At this point, 

```
Evaluate with 3 variables (called 2 times) ->
    Scan has:child (_,_) -> (_0,_1) (called 2 times) ->
        Write (_0,_1) into has:descendent (called 4004 times)
    Scan has:child (_,_) -> (_0,_2) (called 2 times) ->
        Join has:descendent (_2,_) -> (_,_1) (called 4004 times) ->
            Write (_0,_1) into has:descendent (called 37952 times)
```


Two problems:
1. We don't need to call the first branch on each iteration.


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
