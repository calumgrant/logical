# Work plan

# Sum syntax

```
sum M, X in money M has value V.
```


## Short term
- `f X and g X if ...`
- `exit`
- Sum
  - How is sum supposed to work if we need to ensure uniqueness? `sum X in `
- Think about predicate-names containing `-`.
  - Problem is negative facts like `print -2.` Try to turn it into an entity.
  - `error "negative" if not 5 has negative -5.` does not work.
- Binding issues
  - `number X has square X*X.` is not bound yet.
  - `number X has square Y if Y = X*X.` also not bound yet.
  - Report the variable name in the unbound case.
  - Binding tests
Queries that work: `find succeeded _ if 1=1.`
  - Deduplicate results
  - Sort results
- Bug with computed bounds 1<X<Y+1 for example.
- `is not` syntax.
- const fields more

Code refactoring:
- Split up files
- Put into `namespace Logical`

## Datalog predicates

Perhaps mix Datalog syntax with Logical syntax.
Syntax for all, count etc.

## Quantifiers

- [ ] count
- [ ] sum
- [ ] all

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