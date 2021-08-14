# Overview of the Logical language

This page gives a condensed overview of the *Logical* language.

*Logical* is a logic programming language based on Datalog. It offers the ability to script data processing tasks, by offering a rich API to read data from files and databases, transform and query the data, then write out the results in a variety of file and database formats. I would like to think of Logical as a data processing language.

As well as implementing regular Datalog syntax, *Logical* provides a natural syntax, which should hopefully make it easier to use and understand, and might make it suitable for teaching logic programming. Datalog and Logical syntax can be used interchangeably in Logical programs.

## Syntax comparison

Here is the implementation of a `pensioner/1` predicate (`/1` indicates that this is a *unary* predicate), with rules to say that a person is a pensioner if they are male and are 65 or older, or they are female and are 60 or older.

In Datalog, you would write

```
pensioner(X) :- person(X),
    (gender(X, "female"), age(X, A), A>=60; gender(X, "male"), age(X, A), A>=65).
```

the same predicate in Logical would be

```
X is a pensioner if
    person X has gender "female", age A and A>=60 or
    person X has gender "male", age A and A>=65.
```

or

```
if
    person X has gender "female", age A, and A>=60 or
    person X has gender "male", age A and A>=65
then
    X is a pensioner.
```

The Logical interpreter accepts all of these variants. In [CodeQL](https://codeql.github.com),

```
predicate pensioner(Person x)
{
    x.getGender() = "female" and x.getAge() >= 60 or
    x.getGender() = "male" and x.getAge() >= 65
}
```

or

```
class Pensioner extends Person
{
    Pensioner()
    {
        this.getGender() = "female" and this.getAge() >= 60 or
        this.getGender() = "male" and this.getAge() >= 65
    }
}
```

Note that Logical and CodeQL can be translated to Datalog in a fairly straightforward way.

Suppose we wanted to introduce the classes `male` and `female` as their own concepts - perhaps they would be useful for other things as well.

In Logical, this would be:

```
X is male if person X has gender "male".

X is female if person X has gender "female".

X is a pensioner if
    female X has age A and A>=60 or
    male X has age A and A>=65.
```

In Datalog this would be:

```
male(X) :- person(X), gender(X, "male").

female(X) :- person(X), gender(X, "female").

pensioner(X) :-
    female(X), age(X,A), A>=60;
    male(X), age(X, A), A>=65.
```

In CodeQL, this would be:

```
class Female extends Person
{
    Female() { this.getGender() = "female" }
}

class Male extends Person
{
    Male() { this.getGender() = "male" }
}

class Pensioner extends Person
{
    Pensioner()
    {
        this instanceof Female and this.getAge()>=60 or
        this instanceof Male and this.getAge()>=65
    }
}
```

For people familiar with object-oriented syntax, the CodeQL syntax reads very well, but perhaps the Logical syntax reads better for non-experts. CodeQL is a little more verbose but organises predicates nicely into classes.

By they way, this is obviously just an example and is not how the UK tax system really works, or how we should think about gender. But suppose we needed to add a third gender, `unspecified`. In Logical, you can just define further clauses, for example

```
X is a pensioner if
    person X has age A and
    (X has no gender or X has gender "unspecified") and
    A>=65. 
```

There are all sorts of variations possible, for example `X is not female`.

# Basic concepts

Logical programs are organised into *facts*, *rules* and *predicates*. Predicates are things that can be deduced to be true, either directly because they have been stated as facts, or indirectly because they have been deduced through rules.

In Logical, all predicates are *finite*. This means that they can be written out and stored as a table.

The main types of predicates are *unary* predicates (that establish sets of objects), and *binary* predicates (that establish relationships between pairs of objects). For example `male/1` is a unary predicate, and `age/2` is a binary predicate. But is it possible to define other "arities" of predicates, defining relationships between an arbitrary number of things.

A Logical program consists of a list of *statements*, where each statement states either a fact or a rule.

## Facts

A fact is something that is defined to be true, either because it was explicitly defined to be true, or because the data was read from a file or a database.  For example

```
"Fred" is a person.
"Fred" has age 22.
```

The `a` is optional, and is only used to make the code read better.

The following are equivalent:

```
"Fred" is a person.
"Fred" is person.
"Fred" is an person.
person "Fred".
person("Fred").  // Regular Datalog syntax
```

The following are also equivalent:

```
"Fred" has age 22.
"Fred" has an age 22.
"Fred" has a age 22.
age("Fred", 22).  // Regular Datalog syntax
```

When you assert a fact to an extern, it has the effect of "calling" the extern, which can have side-effects including 
asserting facts from a variety of data-sources. 

For example:

```
load-module mysql.
import "facts.dl".
csv:read "data.csv".
parser:parse "main.java".
mysql:connect "Customer".
```

It is possible to assert multiple facts in the same statement. For example

```
male person "Fred" has age 22, gender "male", surname "Bloggs".
```

This is the same as writing out

```
male "Fred".
person "Fred".
"Fred" has age 22.
"Fred" has gender "male",
"Fred" has surname "Bloggs".
```

## Data types

Logical is untyped, meaning that any predicate can store any data. Whilst this is more flexible, it means that you can make mistakes such as `"Fred" has age "22"` or `"Fred has name 22`.

The basic data types in Logical are: integers, floats, strings, at-strings and bools.

## Rules

Rules allow predicates to be conditionally true, by defining relationships between predicates.


# Queries





# Predicates


## Facts and rules

## Unary predicates

Unary predicates define classes of things.

## Data types

## Binary predicates


## Entity clauses


# Datalog syntax

# Logical syntax

# Facts
Facts define the data.

```
// Defining a single unary predicate
"Fred" is a person.
3.14 is a number.

// Combined predicates define many predicates in the same statement
@Mickey is a large mouse.
@Mini is a small red mouse.
aggressive cat @Tom.
@Tom is an aggressive cat.

// Attributes define n-ary predicates
large dog @Pluto has name "Pluto", colour @brown.
colour @brown has red 168, green 109, blue 50.  // List of attributes (binary predicates)
colour @red, red 255, green 0, blue 0.  // Omitting the "has"
```
Entities are values that can be stored in predicates. They can be strings (`"Tom"`), at-strings (`@Tom`), integers (`123`), floats (`3.14`) or Boolean (`true`|`false`).

# Rules
There is no distinction between extensionals and intensionals in Logical. All predicates can have both facts and rules. Predicates with no facts or rules are empty.

Simple rules:
```
X is large if X is human.
large X if human X.
if X is human then X is large and X is an omnivore.
if X is a tiger then X is a large predator.

A has predator B if animal A is small and animal B is large.

A has predator B if 
    A is an animal and
    A is small and
    B is an animal and
    B is large.

B has food A if A has predator B.
A is a herbivore if A has food B and B is a plant.

A is a herbivore if A has food @salad or A has food @cabbage.
```

Negation and range:
```
A is a herbivore if A has food F and F is not meat.

P is unemployed if not P has job _ and P has age A and 18<=A and A<65.
P is unemployed if P has no job and P has age A and 18<=A<65.

// The range syntax is binding
n is prime if 
    2<=n<=1000 and
    not (2<=m<n and n%m=0).

n is prime if 
    2<=n<=1000 and
    all (n%m!=0) in 2<=m<n.
```

Count and sum:


Transitive closure:

```
x has reachable-function y if x has call-to y or.

x has reachable-function y if x reaches call-to y.
```

## New objects

Classical Datalog only allows data to be queried, but Logical allows new entities to be defined. This is useful for situations where it's not necessary to synthesis an id for the entity, or where 

```
new sum-expression p has type @sum, parent e, index 0. 
```

New 

# Recursion


## Builtin predicates

```
print "Hello, world!".
```

String length


# Execution model

Logical programs run by reading a program file, and reading all of the statements in the file. All facts and rules encountered in the program are added to the relevant predicates (stored as tables in memory). If errors are encounted (for example, syntax errors, binding errors and so on, then the error is reported and execution stops).

After all facts and rules have been loaded, the extern predicates are executed, in the sequence they were read in the file. Extern predicates may have side-effects, such as establishing new facts, or loading more facts and rules (the `import` predicate does this). Another common extern is the `query` predicate that displays its results in a table. Note that `query` and `import` are not keywords in Logical, but are implemented as extern predicates.

Predicates can specify their execution order, for example `import` predicates are executed first, and `query` predicates are executed last. `error` predicates are executed before the `query` predicates. If there are errors, then the execution stops at the current stage.

Effectively, all rules in the input files are "executed". The order of execution is undefined, providing that the extern predicates (with side-effects) are executed in sequence. The only thing that is defined is that all rules in one predicate are fully evaluated before its results are used. There is scope for optimization here - some rules do not need to be executed, and of course rules can be executed in parallel, in principle.

