# Directory layout

# Building the source code

The following commands will build the source code and test it.
```bash
mkdir build
cd build
cmake ../src
make
make test
```

## Building on MacOS

Prerequisites:
- Xcode (for MacOS build tools)
- Homebrew (for `flex` and `bison`)
- CMake

```
brew install flex bison
cmake ../src -DBISON_EXECUTABLE=/opt/homebrew/opt/bison/bin/bison -DFLEX_INCLUDE_DIR=/opt/homebrew/opt/flex/include -DFL_LIBRARY=/opt/homebrew/opt/flex/lib/libfl.a
make
make test
```

As you can see, the versions of `flex` and `bison` that ship with the MacOS build tools are out of date and you will need the latest version, that can be supplied using Homebrew.

The default CMake generator is to produce a Makefile, which works perfectly well.  Additional options to CMake include

- `-DCMAKE_BUILD_TYPE=Debug` for a debug build
- `-GXcode` for development in Xcode. Open the generated file `logical.xcodeproj` in Xcode.

# Running tests

In the build directory, run

```bash
make test
```

# Adding tests

New tests are added by adding a `.dl` file to the `tests` directory, and adding an entry for the test in `src/CMakeLists.txt`.

# Overall design

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