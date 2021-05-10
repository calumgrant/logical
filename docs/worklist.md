# Work plan

person @p1 with clothing "Coat", red 255, green 255, blue 128.
person @p1 with clothing "Pants", red 0, green 0, blue 255.

Is this any better than the "has" syntax. Should the "Has" syntax work for this?

- The "comma" creates a tuple that is not binary.

Open questions:
1. Can we project these easily? E.g. `person A has red R`. So, each time we create a tuple, we also create a projection. The name of the projection is `clothing:red` or `clothing:red:green:blue` where the order of the columns does not matter - the same name cannot be repeated, but `clothing:red` and `red:clothing` mean the same thing.
2. Projections can queried individually, for example `@p1 has red R`
3. Projections are populated by rules, and are evaluated lazily.
4. For a large tuple, there are 2^n projections, which isn't exactly great. So, all projections are computed lazily, and need `Database` support.


Createa a projection
`person _ has red R, green G.`



Database.GetRelation(projection)
- Creates a relation if it does not exist
- Adds rules to all necessary projections. The rule `A` has a projection to `A-x`.
  - EvaluateFs -> WriterBs

## Short term
- The `Unique` evaluations do not need a "target" slot at all.
- Use two column deduplication for sum.
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

- Warning about empty predicates with no facts or rules.
- Optimization: Tables should assume a single type, then fall back onto polymorphic behaviour which is slower.

Code refactoring:
- Split up files
- Put into `namespace Logical`

# Unresolved issues
- How does the `with` syntax work
- Putting `-` into identifiers?

## Datalog predicates

- nary predicates
- `with` predicates - how do they work?
Perhaps mix Datalog syntax with Logical syntax.
Syntax for all, count etc.

## Quantifiers

- [ ] sum- [ ] all

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