# Work plan

## Short term
- Implement n-ary predicates
  - Refactor `has` grammar into a vector
  - `Database::getCompoundRelation(CompoundName)` to get the relation and implement the rules
  - Implement n-ary tables
  - Query n-ary relations.
    - Efficient indexing?
    - Arbitrary joins
- Implement `query` predicate.
- Total number of evaluations count.
  probably best just to have a `static`
- Remove the AST `Visit` functions
- File operations
  - `Database::ReadBinary()`
  - `Database::WriteBinary()`
- Implement Datalog syntax
  - unary and binary relations
  - all and count syntax
  - tests for these
  - n-ary relations
    - n-ary tables
    - efficient join orders.
  - how to name arbitrary n-ary tuples from Datalog that are compatible with logical?
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
Queries that work:
  - `find succeeded _ if 1=1.`
  - Deduplicate results
  - Sort results
- Missing input files are not displayed as errors.
- `is not` syntax.
  - `X is not a person`.
  - `X has no job`
- const fields more
- Output timing information
- Output number of errors.
- `not` should fail early (optimization) - see the primes1.dl example
- `or` join both branches if the same variables are bound in all branches.
- Ressurect the ramp and persist projects. Probably persist mainly.
- The indent should be 1, not 4.
- Nicer closure syntax.
- Find syntax? `query X if` is evaluated and displayed immediately.
  - `query X if foo X.`
  - `query X has name Y if`
  - `if 1<=X<100 then find X has attribute X+1.`
  - `query "name" has item X, child Y if Y has parent X`
  - `if Y has parent X then query "name" has item X, child Y.`
  - It would be nicer in general to just use the same syntax.
  - Queries are run at the end, after all rules have been defined.

- Warning about empty predicates with no facts or rules.
- Optimization: Tables should assume a single type, then fall back onto polymorphic behaviour which is slower.

Code refactoring:
- Split up files
- Put into `namespace Logical`
- Slim down the header files, for example split up relation.hpp

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
- `with` predicates - how do they work?
Perhaps mix Datalog syntax with Logical syntax.
Syntax for all, count etc.

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
