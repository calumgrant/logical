# The Logical Reference Manual

Version: 2020-05-06

## Running Logical

## Building Logical

## Running tests

## Command line arguments



# The Logical Language

## Unary predicates

A *predicate* defines something to be true, and a predicate in Logical is simply a table of data. A *unary predicate* is a table with a single column and a variable number of rows. If the table is empty, then the predicate is said to be empty and contains zero rows.

Predicates can be defined using data, or rules. Unlike regular Datalog, Logical defines its rules using a natural syntax.

Facts are statements that simply define a certain thing to be true, and add the values directly to the table. Here are some facts:

```
"Fred" is a person.
3.14 is pi.
@ape1 is an ape.
```

`is`, `a` and `an` are keywords. `a` and `an` are optional and are used to make the statement read more naturally. It is also possible to write the predicate first, as follows:

```
person "Fred".
pi 3.14.
ape @ape1.
```

In these statements, `person`, `pi` and `ape` are unary predicates, and `"Fred"`, `3.14` and `@ape1` are entites that are stored in the table.

## Entities

An entity is something that is stored in a table, and represents one of the values in a Logical program. The types of entity are

* Strings, for example `"abc"`. Strings can contain special escape characters `\"`, `\t`, `\n`, `\r`.
* Integers, for example `1234`.
* Floating point numbers, for example `3.14`. Numbers are in IEEE notation, however they must not end in a trailing dot.
* @-entities, for example `@ape1`. These can be used for synthesised identifiers, and can contain any character except `.` and whitespace. They can also consist of a single `@`.
* Boolean values, `true` and `false`.

In Logical, predicates are untyped, and it is possible to mix any type of data in a table, for example

```
person "Fred".
person @person1.
```

## Compound predicates

Logical supports an unusual syntax where multiple predicates can be defined at the same time. For example

```
large mouse "Mickey".
small red mouse "Mini".
```

alternatively,

```
"Mickey" is a large mouse.
"Mini" is a small red mouse.
```

This is equivalent to writing out

```
"Mickey" is large.
"Mickey" is a mouse.
"Mini" is small.
"Mini" is red.
"Mini" is a mouse.
```

Remember, `a` is simply syntactic sugar and can be omitted.

Compound syntax also appears in rules, where `X is a large mouse` means `X is large and X is a mouse` - more on that later.

Note that `large-mouse "Mickey"` and `large mouse "Mickey"` mean different things. The first defines a single predicate called `large-mouse` and the second defines two predicates `large` and `mouse`.

## Rules with unary predicates

## Binding

## Special predicates

## Negation

## Rules

## or

## and

## Binary predicates

## Datalog syntax

## Recursion

## Transitive closures

Because the transitive closure pattern is so common, Logical has introduced a special keyword for it.
