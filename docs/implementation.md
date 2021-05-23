# The implementation of the Logical programming language

This document contains in-depth implementation notes.

## Lexical Analysis

## Parsing

## Compilation

## Execution model

## Table implementation

## Persistence layer

## Recursion

When predicates 

Mutual recursion: when a predicate is called on the recursive loop, it is called using `QueryRecursive`. This happens if the predicate needs to perform additional evaluation? This then checks the recursive loop to see whether it needs to perform additional recursive steps.....

(We can also use the "on recursive loop" flag to check)

Deltas in mutual recursion:

Predicates need to query the `recursiveRoot->Iteration()` in order to see if they need to perform more steps.


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