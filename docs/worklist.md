# Work plan

## Thinking about n-ary predicates and projections

Conclusions:
1. We use the `has ,` syntax to create n-ary predicates.
2. They automatically project to smaller predicates as needed.

Implementation:
- Database::GetRelation(const std::vector<int> & parts)
- If it's a new relation
  - Look for existing relations to add rules to. Find all relations where we are a superset, and create a rule from ourselves to the subset. Find all relations where we are a subset, and create a rule from the superset to ourselves.
  - Ensure we don't add ourselves twice.

- Data structure:
  unordered_multimap<int, Relation> to look up relation parts.
- Use Relation::ProjectsTo(const Relation&) to detect the interrelationship.
  std::vector<int> to contain the "set" of name parts. (Using the string pool).

## Short term
- Refactor aggregates
  - Have Evaluations for constants?
  - Suprious assignment in rule5.dl
- Implement n-ary predicates
  - Refactor `has` grammar into a vector
  - Implement `CompoundName` which is a vector of int. - Tests
  - `Database::getCompoundRelation(CompoundName)` to get the relation and implement the rules
  - Implement n-ary tables
  - Query n-ary relations.
    - Efficient indexing?
    - Arbitrary joins
- File operations
  - `Database::ReadBinary()`
  - `Database::WriteBinary()`
- Implement Datalog syntax
  - unary and binary relations
  - all and cound syntax
  - tests for these
  - n-ary relations
    - n-ary tables
    - efficient join orders.
  - how to name arbitrary n-ary tuples from Datalog that are compatible with logical?
- Think about `with` syntax
  - Solution 1: construct an nary predicate using `:`. You must specify all of the fields
- `f X and g X if ...`
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
- Bug with computed bounds 1<X<Y+1 for example.
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

When recursion is detected, the predicate is tagged with "recursive evaluation". Then, the rules of the predicate are reevaluated until there are no more results.



- Can we avoid unnecessary branches in the recursive step??
- Can we just use the deltas??

- 



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
