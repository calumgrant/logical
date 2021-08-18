# Overview of the Logical language

This page gives a condensed overview of the *Logical* language.

Logical is a logic programming language based on Datalog. It offers the ability to script data processing tasks, by offering a rich API to read data from files and databases, transform and query the data, then write out the results in a variety of file and database formats. I would like to think of Logical as a data processing language.

As well as implementing regular Datalog syntax, Logical provides a natural syntax, which should hopefully make it easier to use and understand, and might make it suitable for teaching logic programming. Datalog and Logical syntax can be used interchangeably in Logical programs.

# Core language



## Program structure

Logical programs consist of a series of statements, where each statement is a *fact* or a *rule*. Facts define something to be directly true, for example

```
"Fred" is a person.
primary colour @blue has red 0, green 0, blue 255.
```

Rules define things to be conditionally true, for example

```
P is a minor if person P has age A and A<18..
C is visible if colour C has red R, green G, blue B and R+G+B>50.
```

*Predicates* define things that are true, and can be established through facts and rules. Predicates are in general *extensible*, meaning that facts and rules can be added to them at any time. It might be a good idea to group the statements about a predicate together, but it is not required.

To get Logical programs to actually do something, some predicates have side-effects. For example, the `print` predicate prints a line of text, `error` reports an error message, and the `select` predicate displays its contents in a table.

Here are some examples:

```
load-module mysql.

import "generated-data.dl".

print "Read all the data files".

error "Failed to read data" if not datafile _.

select Name N, Red R, Green G, Blue B if
    colour C has name N, red R, green G, blue B.
```

## Facts

A fact is something that is defined to be true, either because it was explicitly defined to be true in the program, or because the data was read from a file or a database.  For example

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

When you assert a fact to an extern, it has the effect of "calling" the extern, which can have side-effects including asserting facts from a variety of data-sources. 

For example:

```
load-module mysql.
import "facts.dl".
csv:read "data.csv".
parser:parse P if source-file P.
mysql:connect "Customer", user "root", password "letmein".
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
"Fred" has gender "male".
"Fred" has surname "Bloggs".
```

or in Datalog syntax,

```
male("Fred").
person("Fred").
age("Fred", 22).
gender("Fred", "male").
surname("Fred", "Bloggs").
```

Facts do not contain variables, so if a fact does contain a variable, then it is treated as a string.  For example

```
import mysql.
load-module data.
```

Remember that `import` and `load-module` are just predicate names, which happen to be external. `mysql` and `data` are treated as strings. It would be just as valid to write `import "mysql"` for example.

## Data types

Logical is untyped, meaning that any predicate can store any data. Whilst this is more flexible, it means that you can make mistakes such as `"Fred" has age "22"` or `"Fred" has name 22`.

The built-in data types in Logical are: integers, floats, strings, at-strings and bools.

## Rules

Rules define logical relationships between predicates. They are written in the form `if` ... `then` ... or ... `if` ... .

```
if X is a dog then X is an animal.

X is an animal if X is a dog.
``` 

in Datalog syntax, this would be

```
animal(X) :- dog(X).
```

*Nonary* predicates (predicates with no arguments that are either true or false) are possible, for example

```
deep-analysis if config "deep-analysis" has value with lowercase "true".
X is a foo if deep-analysis and X is a bar.
```

in Datalog, nonary predicates are written using `()`, for example `deep-analysis()`.

## Variables

Rules can have variables, which can be written in upper- or lower-case. Whether an identifier is a variable or a predicate is dependent on the syntax. The underscore `_` is an anonymous variable, and they do not bind to anything.

Variables must be correctly bound. For example

```
foo X if bar Y.  // Binding error
```

is incorrectly bound because it is not specified what `X` is, so it cannot be stored in a table in Datalog semantics.  All variables are bound by clauses. Some arithmetic operators and extern predicates require their variables to be bound, for example

```
X has negative Y if X = -Y.  // Unbound

X has negative Y if 1<=X<=10 and Y = -X.  // Ok
X has negative -X if 1<=X<=10.  // Ok
```

Binding occurs left to right, so a variable must be bound before it is used.

## Structured entities

The `has` keyword is used to establish or query relationships between entites.

has-clauses

## Logical contructs

and, or, all

## Aggregates





# Library predicates

Logical tried to implement as much as possible in extern predicates - predicates implemented in external C++ modules. This helps to keep the core language smaller, and keeps the 

## The standard library

The standard library, implemented in `stdlib`, is always loaded.

* `any`
* `error`
* `import`
* `load-module`
* `none`
* `print`
* `select`
* `string:length`
* `string:lowercase`
* `string:uppercase`

Examples:

```
load-module mysql.

print "Welcome to the Acme data processor."
print "Finished loading data".
print "Read " + n + " files" if n = find count f in (file f).
```

## Queries

Logical does not have a separate syntax for queries, but instead uses extern predicates that output the results to the console. The nice thing about this approach is that extern predicates can implement different ways to output data, for example to csv files, Markdown or database tables.

The most basic query predicate is called `select` that outputs the results in a tabular form.  For example

```
select Name n, Surname s, Age a if
    n = "Fred" and s = "Bloggs" and a = 22 or
    n = "Joe" and s = "Bloggs" and a = 30.
```

This outputs

```
┏━━━━━━┯━━━━━━━━━┯━━━━━┓
┃ Name │ Surname │ Age ┃
┣━━━━━━┿━━━━━━━━━┿━━━━━┫
┃ Fred │ Bloggs  │ 22  ┃
┃ Joe  │ Bloggs  │ 30  ┃
┗━━━━━━┷━━━━━━━━━┷━━━━━┛
```

`select` is not a keyword, but a regular extern predicate name that outputs its contents to the console. `select` gets the column headings from the attribute names. Attribute names can be strings, so can therefore contain spaces as follows:

```
select "First name" n, "Last name" s, "Age in years" a if
    n = "Fred" and s = "Bloggs" and a = 22 or
    n = "Joe" and s = "Bloggs" and a = 30.
```

this outputs

```
┏━━━━━━━━━━━━┯━━━━━━━━━━━┯━━━━━━━━━━━━━━┓
┃ First name │ Last name │ Age in years ┃
┣━━━━━━━━━━━━┿━━━━━━━━━━━┿━━━━━━━━━━━━━━┫
┃ Fred       │ Bloggs    │ 22           ┃
┃ Joe        │ Bloggs    │ 30           ┃
┗━━━━━━━━━━━━┷━━━━━━━━━━━┷━━━━━━━━━━━━━━┛
```





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