# Introduction
*Logical* is a programming language for writing logical queries in a simple and intuitive way. It is mainly intended as an instructional language to learn about logic.

# Stating facts
A fact is something that is stated to be true, for example

```
"Fred" is a person.
"Fred" has mother "Natasha".
```

In these statements, `"Fred"` and `"Natasha"` are entities, and `person` and `mother` are relations. You can think of relations as tables of data with one or two columns. `person` is a table/relation with one column, and `mother` is a table/relation with two columns.

You can combine statements about a single entity, for example 

```
person "Fred" has mother "Natasha", age 99, surname "Brooks".
```

This defines 4 facts:

1. "Fred" is a person,
2. "Fred" has mother "Natasha",
3. "Fred" has age 99,
3. "Fred" has surname "Brooks".

Technically, `person` is a unary relation, and `mother`, `age` and `surname` are binary relations. 

Entities can be one of the following:

- An integer, (for example `123`)
- A floating point number, (for example `3.14`)
- A string, (for example `"Natasha"`)
- A Boolean value (`true` or `false`),
- A name or number prefixed with `@`, (for example `@xx-yy-123`).

The order in which facts are defined has no significance, except in the case of special predicates.

# Creating rules
A *rule* is a relation that is computed using logic. A rule is a statement that defines something to be true. For example,

```
X is a person if X has parent Y.
X is a person if Y has parent X.
```

This defines a unary relation called `person`, that is determined based on the (binary) relation `parent`.

It is possible to write rules the other way, putting the `if` first, for example

```
if X has parent Y then X is a person.
if Y has parent X then X is a person.
```

In these examples, `X` and `Y` are variables, `mother` is a binary relation and `person` is a unary relation.

If `mother` and `person` are not defined, then they are assumed to be new relations that are initially empty. Variables can be upper- or lower- case, but we will use upper case here.

Another example, this time using `or`.

```
X has parent Y if X has mother Y or X has father Y.
```

This second example adds another condition, namely that `X` is a `person`, but only in the left hand branch (when `X` has a `mother`).
	
```
X has parent Y if person X has mother Y or X has father Y.
	
X is retired if 
    X has sex "male", age A and A>=65 or
    X has sex "female", age A and A>=60.
```

The comma is used to bind multiple attributes to an entity. Without the comma, the previous example would be written out as:
	
```
X is retired if 
    X has sex "male" and X has age A and A>=65 or
    X has sex "female" and X has age A and A>=60.
```

As expected, `and` has a higher precedence than `or`, so there is no need to group these clauses, but it can make things clearer to group them using brackets.

```
X is retired if 
    (X has sex "male" and X has age A and A>=65)
    or
    (X has sex "female" and X has age A and A>=60).
```

`person X`, `X is person` and `X is a person` mean exactly the same thing, but `person X` can have additional attributes. For example

```
X is a person and X has age 20
```
can be shortened to
``` 
person X has age 20
```

# Queries
A *query* is used to find results and print them. Queries can be used to report all members of a predicate, for example

```
find person X.
```

Reports all `X` in the unary relation person.
	
Queries can also contain logic, for example

```
find person X if X has age Y and Y>=20.
```

This query does not report the age `Y` because `Y` only appears after the `if`. To report the `age` as well, it needs to go before the `if`, thus 
	
```
find person X, age Y if Y>=20.
```

Any number of columns can be queried in this way for a single entity. To query multiple entities

```
find X, Y, Z if person X, age Y and Z has child X
```

The order in which facts and rules are stated does not make a difference, but a query will only be run on facts that have already been established, and precede the query in the file.

# Comments
Use C or C++-style comments.

# Evaluation model
Logical uses a simple evaluation model where all relations are fully evaluated and are stored as tables in memory. A relation is computed when it is queried, which means running the rules and filling the table.

Relations are computed left to right, and if performance is important, then it is important for the person writing the rule to take evaluation order into consideration. For example

```
person X has age Y
```

is computed by scanning the unary relation `person`, and joining it with the binary relation `age`. To swap the order of this evaluation, you could write
	
```
X has age Y and X is a person
```

This second rule is evaluated by scanning the binary relation `age`, and then filtering it on the unary relation `person`.

There is no attempt to optimize the evaluation order or to remove redundant steps.

# Quantifiers
```
cash @1 has value 10.

"Fred" has cash @1.

X is rich if 
    X has cash Y and
    Y has value Z and
    Z>10.

X is rich if
    sum Z in (X has cash Y and Y has value Z) > 10.

X is rich if 
    all Z>10 in 
    (X has cash Y and Y has value Z).
```

# Special predicates

These special predicates have side-effects when facts are added to them. (This includes rules that are evaluated immediately).

## `import`

The `import` predicates imports a file. For example

```
import "testdata.logic"
import "more_rules.logic"
```

## `print`

The `print` predicate displays the string to the output.

```
if
    person X has name N
then
    print "Your name is " + N.
```

## `error`

Like `print`, `error` displays a message, this time to stderr.

```
error "There are no supervisors." if not supervisor S.
```

## `connect`

Connects to a server. For example,

```
connect "localhost", username "foo", password "..."
```

## `session`

Creates a named session, whose results are persisted.

```
session @9847.
import "myfile.logic"
session 0.
```

## `clear`

Clears all results in the current session.

```
clear().
```

## `exit`

Exits the session, closes the connection or exist the command.

```
exit().
exit 0.
exit(0).

exit 127 if not user _.
```
