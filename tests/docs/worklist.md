# Work plan

## Short term
- Binary predicates
- `f X and g X if ...`
- `exit`
- Refactor `AST::EntityIs` into a common subtype `EntityClause`
  - entityOpt `X` in `X is a mouse`
  - unaryPredicates `large`, `mouse` in `large mouse X is ...`
  - definedPredicates `small` and `green` in `mouse X is small, green`
  - AttributesOpt `name Y` and `age 99` in `X has name Y, age 99`
  - WithAttributes `mouse X with age 99, colour @blue`
- Remove `BinaryPredicateOrList` and just use `BinaryPredicateList`

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

## Second release

# Documentation

- [ ] Readme
- [ ] Tutorial
- [ ] Manual
- [ ] Contributing guide, how to build