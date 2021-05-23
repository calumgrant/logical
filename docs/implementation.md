# The implementation of the Logical programming language

This document contains in-depth implementation notes.

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

All predicates in a recursive loop must be iterated, so a recursive pred

As an additional complication, 


```
class RecursiveLoop
{
    // Called by all predicates in the loop when they add results.
    void AddResult();

    // Size NumberOfResults();


    // True if all predicates in the loop
    bool Finished();
};
```


Analysis of recursion: Each predicate has a flag to indicate that it is in a recursive loop




When predicates "call" themselves, they are recursive.

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