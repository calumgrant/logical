# Work plan

How to compile `A=1 and B=2 and C=A+B`.
  The expression does not bind any variables, or it is an error.


- Rename `CompileEntity` to `BindVariables`
- Call `Entity::Compile` That in many cases is a noop.

X=Y+1`:
  - Bind all of the variables in execution sequence
  - 

 - we need to create an evaluation
    Calculate _2 = _1 + _0 ->
        Assign _3 = _2

## Short term
- `number X has successor X+1, predecessor X-1.`
- `f X and g X if ...`
- `exit`
- Comparators in general
- Arithmetic
  - Addition
  - String addition
  - Other numerical operators: - * / % unary-
- Prime number sieve
- Count
- Sum
- All

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