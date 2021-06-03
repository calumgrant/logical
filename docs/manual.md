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

Because the transitive closure pattern is so common, Logical has introduced a special keyword for it. `reaches`

Grammar:

> reaches-clause:
>     unary-predicate-list entity *reaches* binarypredicate entity


## New entities
The `new` keyword creates new entities in the database. This is unlike rules, which only use existing entities, or facts that use hard-coded entities. New entities offer a convenient way to package together a set of attributes into a single entity.

`new` can be used in facts as follows:
```
new person has name "Fred", age 99.
```
This creates a new `person` entity that is guaranteed to be unique, and there is no need to synthesise an identifier for it.

`new` can also be used in rules, for example

```
new woman has name n, age a if person p has name n, age a, gender "female" and a>=18.

new parameter has name n, function f, index i if function f has parameter-name n, parameter-index i.

new number has value 0.
new number has value v+1, predecessor p if number p has value v and v<10.

new location has
    file f,
    start-line l1,
    start-column c1,
    end-line l2,
    end-column c2
if
    function _ has file f, start-line l1, start-column c1, end-line l2, end-column c2.
```
As you can see, `new` entities can be defined recursively.

If there are multiple rules, then this will define multiple entities, even with the same attributes.
```
new person has name "Fred".
new person has name "Fred".

new file has name f, parent d if directory d has file f.

new person has name n+" Smith" if person _ has name "Fred".
```

# Grammar

## Lexical analysis