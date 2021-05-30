# The implementation of the Logical programming language

This document contains in-depth implementation notes.

## Overall design

Logical consists of a main executable, `logical`, that reads source files and outputs the results to stdout. There is no persistent state like a database (but this could be an interesting new feature).

The entry point is the `main()` function in the file `main.cpp`. This consists of a simple command line parser, and invokes
flex and bison to parse the source files. The parser is defined in `logical.l` and `logical.y`.
The parser processes as it's going along, as opposed to generating a huge syntax tree and then processing it.
This should add more scalability.

The classes for the abstract syntax tree are defined in `AST.hpp` and `AST.cpp`. This defines the basic program elements.
The main clases here are

 * `class Node` the base class of all syntax nodes.
 * `class Clause` the base class of all clauses.
 * `class EntityClause` the base class of clauses like `X is a mouse`.
 * `class Entity` the base class for nodes representing entities, like `5`, `X` or `"Fred'`.

The AST classes contain methods to compile them. Compilation is performed directly on the AST, and there is no further
translation layer to raw Datalog, or a relational algebra. The product of the compilation is an `Evaluation` class that evaluates a rule.  `AST::Clause::Compile()` compiles the current clause into an `Evaluation` object.

The `Evaluation` class has a method, `Evalaution::Evaluate(Entity * row)` that evaluates a clause in a rule. In general,
an `Evaluation` contains a `next` evaluation, and the results of the evaluation are passed to the `next` evaluation by
calling `next->Evaluate(row)`. `Evaluate` can call `next->Evaluate()` 0, 1 or many times. The final stage in evaluation
is to call a `Writer` class that writes data into a relation. By enabling verbose output (`logical -v`), you can see
the structure of the `Evaluation` classes, which is generated using `Evaluation::Explain()` method.

There are many different `Evaluation` classes. The main classes are:

 * `EvaluateF` scans a unary predicate, and calls `next->Evaluate()` with each row in the predicate.
 * `EvaluateB` checks if a value is in a relation, and calls `next->Evaluate()` once if successful.
 * `EvaluateFF`, `EvaluateFB`, `EvaluateBF`, `EvaluateBB` scans, joins or checks binary predicates.
 * `WriterB`, `WriterBB` writes values into predicates.
 * `OrEvaluation` calls `Evaluate()` on both branches.
 * `EqualsBB`, `EqualsBF` implements the `=` operator.
 * `NoneEvaluation` always fails. This is mainly a placeholder for testing/not implemented.

 Data is stored in relations. The `Relation` class is the main class, which has implementations for unary and binary predicates, as well as the built in predicates. `Relation::Add` adds a row to the relation, and `Relation::Query` queries it. Relations also
 have rule attached to them, that are executed lazily when the relation is queried.

 All values are stored in the `Entity` class. This is a variant type, consists of a datatype field and an integer to store the data. Strings are stored as an index into a string table.

 Everything is stored in the `Database` class. This contains a table of all of the relations, and also contains the string table.


## Lexical Analysis

## Parsing

## Compilation

## Execution model

## Table implementation

## Persistence layer

## Recursion

A *recursive* predicate is one that queries itself, perhaps indirectly.  Two or more predicates that query each other are *mutually recursive*, and in general predicates that query each other form a *recursive loop*.

Predicates that are recursive must be iterated (in a loop) until there are no more results added to any of the predicates, and the loop terminates.

In a predicate, the first recursive call (that is *iteration independent* can be a delta, whereas all subsequent calls are *iteration dependent* and cannot use the delta. A *delta* only queries the *changes* since the last iteration, and since this is much smaller than the original predicate, results in a significant improvement in performance.

All predicates in a recursive loop must be iterated, until the entire loop is complete. When a predicate is queried, it first checks to see if rules need to be run. If the predicate is in a recursive loop, then the predicate checks to see if any more data has been added to the recursive loop. If it has, then the predicate evaluates, and iterates until no more data is added to the loop.

Delta optimization can be applied to the first recursive step in an evaluation.

Analysis of recursion: Each predicate has a flag to indicate that it is in a recursive loop.

Mutual recursion: when a predicate is called on the recursive loop, it is called using `QueryRecursive`. This happens if the predicate needs to perform additional evaluation? This then checks the recursive loop to see whether it needs to perform additional recursive steps.....


Deltas in mutual recursion:

Predicates need to query the `recursiveRoot->Iteration()` in order to see if they need to perform more steps.

## Memory management

Logical uses a special memory allocator for certain operations. See the [Persist](...) library for an overview of this. C++ containers can be wired to use the Persist allocatior using the data types `persist::allocator<>` and `persist::fast_allocator<>`. The difference between these two allocators is that `persist::fast_allocator<>` will not recycle its memory on deletion, which speeds things up, but should not be used for transient data.

C++ containers can be configured to use the Persist allocator by supplying it as a template argument, for example `std::vector<int, persist::fast_allocator<int>>`, and the Persist shared memory reference needs to be passed to the constructor of such containers.

The benefits of the persistent memory allocator are:

- Ability to use much more memory than simply the size of the swap file.
- Ability to monitor and limit memory usage.
- Generally faster than the regular memory allocator.
- Ability to save and restore state very quickly (currently it's broken - need to investigate).

## Optimization

There are three main optimization levels, `-O0`, `-O1` and `-O2`, although the numbers also go up further to enable slower/more experimental optimizations.

* `-O0`: Disable all optimizations, only performing the bare-minimum of analysis to ensure the program is correct. The main purpose of this level is to compare the outputs with `-O1` and `-O2` to check if the optimizer is correct of if the results change. Users can enable this option to help to report bugs. If the results change between different optimization levels, then there is a bug in the optimizer.
* `-O1`: Enable all common optimizations, the default. This performs optimizations that are intended to be fairly fast to perform and will not slow down program compilation very much.
* `-O2`: Enable all optimizations, including optimizations that are a little slower and more complicated.
* `-O3` ...: Advanced optimizations. These could be experimental or run more slowly, but could be a win for big databases where evaluation time is more important.

Individual optimizations can be enabled or disabled using the `-f` or `-fno-` option.

### `-O1` optimizations

`-fdeltas`
`-fsemi-naive`
`-freorder` Reorder joins.
`-finline` Inline evaluations