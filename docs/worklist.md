# Work plan

## Short term

- Time each predicate.
- Better results
 - Sort the results
 - Deduplicate the results
  - Expect a certain number of results. If not, report an error.
     `expect 100.`
  - Implement `find X,Y,Z in`.
- Column names in `find` and `query`.
- Think about CSV output.
- Implement a resultset. It's just a table with a writer. The table stores the results.
- Consider synthesising an ID for `has:Person:Name`

## Refactoring
- Implement a `ChainedEvaluation` class that exposes `Next`?
- Use `typedef int Id`
- Create a `VariableInfo` structure
  - slot
  - bound
  - last use?
  - With pretty printing?
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
- [ ] MySQL connector.
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
