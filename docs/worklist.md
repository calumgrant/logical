# Work plan

## Short term
- `f X and g X if ...`
- `exit`
- Arithmetic
  - Other numerical operators: - * / % unary-
- Prime number sieve
- Count
- Sum
- All
- Think about predicate-names containing `-`.
- Problem is negative facts like `print -2.` Try to turn it into an entity.
- `number X has square X*X.` is not bound yet.
- `number X has square Y if Y = X*X.` also not bound yet.
- Tuple counts on all `Evaluate()` methods.
- `error "negative" if not 5 has negative -5.` does not work.


## Binary predicates

## Datalog predicates

Perhaps mix Datalog syntax with Logical syntax.
Syntax for all, count etc.

## Quantifiers

- [ ] count
- [ ] sum
- [ ] all

## Ranges

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