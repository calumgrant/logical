# Significant differences to Datalog

The main difference between Datalog and 

The main difference is the ability

1. Combinations of predicates

Writing to multiple predicates at the same time.

It is possible to define multiple predicates in a single rule. For example
```
large cat "Tom".

small cat "Felix" has lives 9, age 5.

dog X has owner Y if
    X has owner P and P has spouse Y.
```
