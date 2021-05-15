# Work plan

## Short term
- Compound names aren't displayed properly in Explain.
- Colour-code the output explanations.
  - Refactor this properly
- Remove `NotTerminator` and not terminator to use a `Load None`
- Create a `VariableInfo` structure
  - slot
  - bound
  - last use?
- `expect` predicate - number of evaluation steps. Prints a message
- `steps` predicate - gets the current number of evaluation steps
  `if S is steps then print "Currently at " + S + " steps".`
  `print "..." if steps S.`
- Adding strings to ints??
- Reporting options
  - `-q` for quiet
  - `-v` for verbose
  - By default, display steps and time, and number of results and number of errors.
  
  Found 123 results.
  Evaluation finished with 1 errors.
  Evaluation finished.
  - Parser error recovery with `.`
- Implement n-ary predicates
  - Assert rules
  - Query
  - Query n-ary relations.
    - Efficient indexing?
    - Arbitrary joins
- Implement `query` predicate.
  result p has message "Unused parameter." if p is a parameter and parameter has no use.
  find result -> Looks at all tuples involving result as well.

  
   if X is a parent
- What about object-orientation?
- Warn on empty predicates
- Perhaps have an Evaluation::SetRow() so that it's possible to store the row?
- Report duplicate attributes a bit better.
- Count total number of rows stored.
- Warning on undefined predicates.
- Remove the AST `Visit` functions
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
- Free intermediate tables, for example tables used for deduplicating.
- Is the deduplicating logic even sound?? Surely other variables can change too? So we need to deduplicate lots of variables, not just the ones in the sum.
- Problem with adding rules on demand if a predicate is already being evaluated.
- Could `large mouse @mickey` actually mean `large-mouse @mickey`
- Has with no entity? For example `query has message "Hello"` if `query` is unbound then it's taken to be a predicate name???

Code refactoring:
- Split up files
- Put into `namespace Logical`
- Slim down the header files, for example split up relation.hpp

## Class system

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
- [ ] MySQL connector.
- [ ] Dynamic-linked libraries
- [ ] Reading external data.

# Optimizer
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
