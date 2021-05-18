#pragma once

#include "Fwd.hpp"

struct OptimizationOptions
{
    // -O1 options
    
    // Detect and report errors with negative recursion
    bool findNegativeRecursion = true;

    // Enable predicates to be partially evaluated
    bool semiNaive = true;
    
    // Query the delta instead of the whole predicate.
    bool recursiveDeltas = true;
    
    // If a predicate is empty after evaluation, then replace the
    // evaluation with None as part of the analysis.
    bool pruneEmptyQueries = true;

    // All None branches are pruned.
    // Ors with one None branch are removed.
    bool pruneNone = true;
    
    // Move all loads to the front of the evaluation
    bool liftLoads = false;
    
    // Based on the sizes of the queried predicates, reorder joins
    bool reorderJoins = false;
    
    // Reduces the number of local variables by reusing slots.
    bool optimizeVariableLayout = false;
    
    // Avoid redundant assignments by aliasing varibles instead of
    // assigning them.
    bool aliasVariables = false;
    
    // Finds steps that are redundant because they are always true.
    bool removeRedundantSteps = false;
    
    // When a variable is used last, and there are no reads of the variable,
    // then there is no need to calculate it, and these is no need to query for it.
    // Remove these fields from joins, for example.
    bool detectUnusedVariables = true;
    
    // -O, -O2 options
    
    // -O3 options
    
    // Checks the types of all predicates and the types of variables.
    // A type can either be None: (no values), a given type, or Any.
    // Remove empty joins based on types.
    // Optimize arithmetic operations if we know the types ahead of time.
    bool analyseTypes = false;
    
    // Require that all tables store a single datatype, and
    // warn if a column contains more than one type.
    bool singleTypedTables = false;
    
    // When a predicate has been evaluated, it is no longer valid to add data and rules to it.
    bool sealedPredicates = false;
        
    bool incrementalEvaluation = false;
    
    bool inlinePredicates = false;
    
    bool magicSets = false;
    
    // Something about deduplication and uniqueness.
    
    bool sizeEstimates = false;
    
    bool mergeBranches = true;
};

void AnalyseRecursion(Database & db, EvaluationPtr root);
